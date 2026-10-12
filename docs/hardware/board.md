# LILYGO T-Camera S3 V1.6

The current firmware configuration targets the LILYGO T-Camera S3 V1.6 with
an ESP32-S3 and OV2640 camera.

| Specification | Project configuration / recorded identification |
| --- | --- |
| MCU | ESP32-S3 |
| Flash | 16 MB, selected in the ESP-IDF project configuration |
| External PSRAM | 8 MB, reported by the boot log |
| Camera sensor | OV2640, detected by the camera driver |
| Framework | ESP-IDF 5.5 |
| Camera output | QVGA 320 × 240 JPEG for current validation |
| Camera XCLK | 20 MHz |

The pin mapping is defined in
[`components/board_support/include/board_config.h`](../../components/board_support/include/board_config.h).
The board revision matters: use the V1.6 mapping and consult the board
manufacturer's documentation before applying it to another revision.

See [Pin configuration](/hardware/pins) and
[Camera, OLED & PMU](/hardware/peripherals).
