# Hardware requirements

The documented target is the LILYGO T-Camera S3 V1.6 assembly. The current
recorded hardware run used the camera board with its OV2640, OLED, and PMU.

| Item | Verified configuration or reported identification |
| --- | --- |
| Board | LILYGO T-Camera S3 V1.6 |
| MCU | ESP32-S3 |
| Camera | OV2640 |
| Flash | 16 MB configured in the project |
| External PSRAM | 8 MB; reported detected by the firmware |
| OLED | SSD1306, I²C address `0x3C` reported by firmware |
| PMU | AXP2101, chip ID `0x4A` reported by firmware |

The OLED address and PMU ID indicate communication/identification. They do not
by themselves prove visual display output or comprehensive PMU behavior.
See [Camera, OLED & PMU](/hardware/peripherals) for details.

## Board-specific wiring

Use the board's existing pin mapping; do not infer connections from unrelated
LILYGO revisions. See [Pin configuration](/hardware/pins).
