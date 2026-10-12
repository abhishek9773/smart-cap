# T-Camera S3 Camera Pipeline

ESP-IDF firmware for the camera capture and JPEG preprocessing pipeline, with
optional SSD1306 OLED and AXP2101 PMU support, on the LILYGO T-Camera S3. The
pin mapping follows LilyGO's official repository; the current configuration
targets the V1.6 board.

## Project layout

```text
components/
├── axp2101/          PMU chip-ID driver
├── board_support/    Board pinout and hardware configuration
├── camera_driver/    Reusable camera lifecycle and latest-frame capture
├── camera_pipeline/  Camera capture plus image preprocessing orchestration
├── image_preprocessor/ Bounded JPEG decode, resize, and pixel conversion
├── i2c_bus/          Shared I2C bus lifecycle and register access
└── ssd1306/          OLED initialization and test-pattern driver
main/
└── main.c            Application startup and component orchestration
```

Each hardware module is an ESP-IDF component with its own source, public
header, and `CMakeLists.txt`. Keep board-specific pin assignments in
`components/board_support/include/board_config.h`.

## Build and flash

Use an ESP-IDF 5.5 environment configured for the `esp32s3` target:

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Use the port assigned to the connected board if it is not `/dev/ttyACM0`.
To monitor a firmware image that is already flashed, run:

```sh
idf.py -p /dev/ttyACM0 monitor
```

## Runtime behavior

- OLED and PMU checks are optional; camera capture still starts if I2C
  initialization or either peripheral check fails.
- The PMU chip ID is read before any camera supply configuration. If it
  matches the AXP2101, only LilyGO's factory camera rails are configured:
  ALDO1 at 1500 mV, ALDO2 at 3000 mV, and ALDO4 at 3000 mV. Other PMU rails are
  not modified.
- LilyGO's separate minimal-camera example uses a different ALDO1/ALDO2
  voltage pair. This setup follows the full factory power configuration; do not
  combine the two presets without confirming the required camera module rails.
- The boot-time preprocessing validation runs for at least 30 seconds at
  QVGA JPEG. It decodes, resizes to 160x120, converts to RGB565, and reports
  processed and failed frames plus output size. It requests each frame only
  when ready, returns the camera-owned frame buffer before JPEG decoding, and
  uses the camera driver's latest-frame mode rather than a timed capture queue.
- The reusable image preprocessor supports configurable output dimensions and
  RGB888, RGB565, or grayscale output. JPEG input, decode pixels, and output
  pixels each have explicit allocation limits; working buffers are allocated
  once and reused. No AI inference is included.

See [docs/hardware-test.md](docs/hardware-test.md) for the 30-second hardware
validation procedure and expected log sequence.

## Board reference

- [LilyGo-Cam-ESP32S3](https://github.com/Xinyuan-LilyGO/LilyGo-Cam-ESP32S3):
  official board versions, examples, and pin tables.
- [LilyGO camera pin table](https://github.com/Xinyuan-LilyGO/LilyGo-Cam-ESP32S3#4%EF%B8%8F-pins):
  camera and shared OLED/PMU I2C GPIO assignments.
- [LilyGO factory power setup](https://github.com/Xinyuan-LilyGO/LilyGo-Cam-ESP32S3/blob/master/examples/factory/power.cpp):
  camera supply rail setup used by this test.
- [LilyGO minimal camera setup](https://github.com/Xinyuan-LILYGO/LilyGo-Cam-ESP32S3/blob/master/examples/MinimalCameraExample/MinimalCameraExample.ino):
  board camera pin mapping and clock configuration.

Check the board revision before changing the pin map. LilyGO notes that V1.6
and V1.7 use different microphone interfaces.
