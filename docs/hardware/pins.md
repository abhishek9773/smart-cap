# Pin configuration

These assignments are copied from the current
`components/board_support/include/board_config.h` for the LILYGO T-Camera S3
V1.6. They are documentation of the existing configuration, not a proposed
rewiring.

## Camera interface

| Signal | GPIO |
| --- | ---: |
| RESET | 39 |
| XCLK | 38 |
| SCCB SDA | 5 |
| SCCB SCL | 4 |
| D0 | 14 |
| D1 | 47 |
| D2 | 48 |
| D3 | 21 |
| D4 | 13 |
| D5 | 11 |
| D6 | 10 |
| D7 | 9 |
| VSYNC | 8 |
| HREF | 18 |
| PCLK | 12 |

## Shared I²C bus

| Signal | Assignment |
| --- | --- |
| SDA | GPIO 7 |
| SCL | GPIO 6 |
| Controller | I²C port 0 |
| Bus frequency | 400 kHz |
| OLED address | `0x3C` |
| PMU address | `0x34` |

Camera SCCB uses the separate SDA/SCL assignments shown above. No pin
configuration is changed by the documentation site.
