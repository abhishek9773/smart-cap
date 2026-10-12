# T-Camera S3 Hardware Test

ESP-IDF firmware for exercising the camera, SSD1306 OLED, and AXP2101 PMU on
the LILYGO T-Camera S3. The pin mapping follows LilyGO's official repository;
the current configuration targets the V1.6 board.

## Project layout

```text
components/
├── axp2101/          PMU chip-ID driver
├── board_support/    Board pinout and hardware configuration
├── camera_test/      Camera capture test and frame-rate reporting
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
  voltage pair. This diagnostic follows the full factory power setup; do not
  combine the two presets without confirming the required camera module rails.
- The camera test runs for at least 30 seconds at QVGA JPEG and reports
  frame dimensions, format, frame length, capture failures, and approximate
  FPS.

See [docs/hardware-test.md](docs/hardware-test.md) for the expected log
sequence and hardware test procedure.

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
