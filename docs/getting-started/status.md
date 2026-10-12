# Project status

The current repository is a camera and image-preprocessing prototype. The
complete assistive Smart Cap system is **not yet finished** and is not
production-ready.

## Implemented and verified in the current firmware

| Area | Current behavior |
| --- | --- |
| Board | ESP32-S3 on LILYGO T-Camera S3 V1.6 |
| Camera | OV2640 configured for QVGA JPEG capture |
| Capture policy | Demand-driven application requests; camera driver uses latest-frame mode |
| Preprocessing | JPEG decode, bounded decode dimensions, resize, pixel-format conversion |
| Output formats | RGB565, RGB888, or grayscale are available in the preprocessor |
| Validation configuration | 320 × 240 JPEG to 160 × 120 RGB565 |
| Optional peripherals | SSD1306 and AXP2101 checked over I²C |

The recorded test results are specific to one run. See
[Recorded results](/testing/results).

## Planned or unavailable

| Capability | Status |
| --- | --- |
| AI inference and object detection | Not implemented |
| Object distance estimation | Not implemented |
| Voice prompts or alerts | Not implemented |
| Navigation decisions and obstacle avoidance | Not implemented |
| End-to-end wearable usability and safety validation | Not completed |

Do not interpret component names, future concepts, or this roadmap as evidence
that a feature has been delivered.
