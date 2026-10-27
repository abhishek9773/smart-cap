# Smart Cap ESP32-S3 prototype

ESP-IDF firmware for the LILYGO T-Camera S3 v1.6. The ESP32-S3 hosts a local
Wi-Fi access point and HTTP API and captures JPEG frames from the existing
camera driver. A future Android application performs on-device object detection
and offline text-to-speech. No cloud service or internet access is required.

This repository is a prototype. Distance sensors, motor drivers, the audio
amplifier interface, and physical switch GPIOs are not yet configured in this
firmware. Their API status is reported as unavailable; firmware does not invent
measurements or claim playback/haptic success.

## Structure

```text
main/
  app_main.c              Application entry point
  app_init.c              Ordered subsystem initialization
components/
  audio_player/           WAV validation and playback capability/status
  board_support/          Verified existing board and camera pin map
  button_manager/         Physical mode-switch availability
  camera_driver/          Existing ESP32 camera lifecycle and frame access
  camera_pipeline/        Existing bounded JPEG preprocessing pipeline
  event_manager/          Bounded event ring
  haptics/                Haptic interface and capability/status
  http_api/               HTTP handlers and API contract
  image_preprocessor/     Existing bounded image preprocessing
  sensor_manager/         Distance and battery availability/status
  system_status/          Runtime, camera, memory, and mode state
  wifi_manager/           Configurable local access point
```

The existing AXP2101, I2C bus, SSD1306, and image preprocessing components are
preserved. The PMU is not written to for battery readings because no verified
battery-measurement path is enabled here.

## Configure, build, and flash

Use ESP-IDF 5.5 and target `esp32s3`:

```sh
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Set the AP SSID, WPA2 password, and IP in **Smart Cap Wi-Fi AP** in menuconfig.
The development defaults are `SMARTCAP_TEST`, `smartcap123`, and `192.168.4.1`;
change the password before deployment. The password is not logged or returned
by the API.

Connect a test client directly to the AP, even if Android reports that the
network has no internet. The local base URL is `http://192.168.4.1`.

## API

JSON responses use `{"ok":true,"data":...,"error":null}` or
`{"ok":false,"data":null,"error":{"code":"...","message":"..."}}`.

| Method | Path | Purpose | Hardware |
|---|---|---|---|
| GET | `/health` | API service health and uptime | No |
| GET | `/api/v1/status` | Runtime, AP clients, camera, audio, haptics, mode, battery availability, memory | No |
| GET | `/api/v1/capabilities` | Features initialized/available in this firmware | No |
| GET | `/api/v1/frame` | Latest JPEG; cache disabled; limited to 2 requests/second | Camera |
| GET | `/api/v1/sensors` | Distance and battery measurements (`null` when unavailable) | No |
| POST | `/api/v1/audio` | Validates bounded mono 16-bit/16 kHz WAV uploads | Audio output required to accept playback |
| GET | `/api/v1/audio/status` | Audio configuration and playback state | No |
| GET, POST | `/api/v1/mode` | Read or set `vibration_only` / `vibration_sound` mode | No physical switch currently configured |
| POST | `/api/v1/haptics/pattern` | Request a supported bounded pattern | Haptic drivers required |
| GET | `/api/v1/haptics/status` | Haptic availability and activity | No |
| GET, PUT | `/api/v1/haptics/config` | Haptic configuration | Haptic drivers required |
| POST | `/api/v1/haptics/test`, `/api/v1/haptics/alert` | Haptic test or alert | Haptic drivers required |
| POST | `/api/v1/command` | `get_status`, `stop_audio`, `stop_haptics` | Varies |
| GET | `/api/v1/events` | Bounded recent event list | No |
| GET | `/api/v1/metrics` | Frame and memory counters | No |

Expected unavailable hardware returns explicit errors such as
`AUDIO_NOT_CONFIGURED` and `HAPTICS_NOT_CONFIGURED`. Invalid or unsupported WAV
data is rejected before that availability check. The HTTP API is local-network
only and does not provide transport encryption.

## API checks

Basic manual checks:

```sh
curl -i http://192.168.4.1/health
curl -i http://192.168.4.1/api/v1/status
curl -i http://192.168.4.1/api/v1/capabilities
curl -f http://192.168.4.1/api/v1/frame -o frame.jpg
```

The device integration checker is `scripts/test_device_api.py`; it talks to a
running device and distinguishes API checks from the hardware-only frame test:

```sh
python3 scripts/test_device_api.py --base-url http://192.168.4.1
```

## Optional vibration-module hardware test

The two logic-input modules connect to GPIO15 (left IN) and GPIO16 (right IN).
The T-Camera S3 V1.6 routes its external 3.3V expansion output from AXP2101
DCDC3. Startup now configures that rail to 3.3V and verifies the PMU register
readback before enabling it; it does not enable unrelated PMU rails. If the
header still measures 0V after boot, check the `board_support` and `axp2101`
serial logs. The repeating test will not start if PMU rail configuration fails.
Do not continue the motor test until the rail is verified; if it remains at 0V
or collapses under load, disconnect the module VCC leads and investigate the
board's power input, PMU, and wiring.
Their exact model documentation was not supplied, so confirm whether `IN` is
active-high or active-low before powering the test. In `idf.py menuconfig`,
open **Smart Cap Haptics**, enable **Enable repeating vibration-module hardware
test**, and select **Motor module IN is active high** according to the module
documentation. The test option defaults off; ordinary boots do not configure or
toggle these GPIOs.

With the test enabled, after normal app initialization the firmware repeatedly
activates the left output for one second, pauses one second, tests the right
output the same way, tests both together for one second, then waits two seconds.
The `haptics` tag prints each test label to the serial monitor. Stop the sequence
without rebooting using:

```sh
curl -i -X POST http://192.168.4.1/api/v1/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"stop_haptics"}'
```

The GPIO pins provide logic signals only. Do not connect a bare motor directly
to an ESP32 GPIO. Confirm that each module's input stage is compatible with
3.3 V logic and that its power supply stays within the module rating. Successful
compilation does not verify physical motor operation.

## Hardware validation still required

- Verify sustained OV2640 frame capture on the actual board.
- Confirm the exact sensor modules and their driver circuits before adding any
  distance-sensor GPIO configuration.
- Confirm MAX98357A wiring and available I2S pins before enabling playback.
- Confirm motor driver circuits and pins before enabling haptics.
- Confirm the mode switch and power switch wiring before enabling button GPIOs.
- Verify battery measurement availability from the exact board schematic and
  identified PMU before reporting voltage, charge percentage, or charging state.

Only the existing camera and shared I2C mappings in
[`components/board_support/include/board_config.h`](components/board_support/include/board_config.h)
are configured here. Do not infer sensor, motor, audio, or switch pin numbers
from this firmware.
