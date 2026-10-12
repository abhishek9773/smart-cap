# Recorded results

The following values are the latest recorded hardware-test results supplied
for this project. They describe that test run and are not long-term or
production guarantees.

## Image preprocessing run

| Metric | Recorded result |
| --- | --- |
| Input resolution | 320 × 240 |
| Input format | JPEG |
| Output resolution | 160 × 120 |
| Output format | RGB565 |
| Output buffer size | 38,400 bytes |
| Test duration | Approximately 30 seconds |
| Successfully processed images | 270 |
| Reported processing failures | 0 |
| Approximate processing throughput | 9 images/second |

Throughput is the observed rate of images processed by the full application
path in this run. It is **not a separately measured camera capture FPS**.

## Board and peripheral evidence

| Check | Recorded observation |
| --- | --- |
| MCU | ESP32-S3 firmware booted |
| PSRAM | 8 MB detected |
| OLED | Responded at I²C address `0x3C` |
| PMU | Chip ID `0x4A` reported |
| Camera sensor | OV2640 detected |
| Camera | Initialized successfully |

An OLED I²C response verifies communication only; no independent visual
verification result was recorded.

## Interpretation

This result demonstrates a successful recorded hardware preprocessing run. It
does not guarantee future runs, extended stability, performance under a
different scene or configuration, production readiness, or complete Smart Cap
functionality. No AI detection, distance estimation, or voice guidance was
tested.

See [Current limitations](/testing/limitations).
