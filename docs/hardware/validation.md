# Hardware validation

The recorded hardware validation was run on the LILYGO T-Camera S3 V1.6. It
established the following reported events:

- ESP32-S3 booted the firmware.
- Firmware detected 8 MB of PSRAM.
- OLED responded at I²C address `0x3C`.
- PMU reported chip ID `0x4A`.
- Camera driver detected the OV2640 and initialized the camera.
- The approximately 30-second preprocessing run completed with 270 images
  processed and zero reported processing failures.

The OLED result confirms I²C communication and a successful test-pattern write
in firmware; it is **not** a documented visual inspection of the display.

## What this validates

This is useful evidence for board bring-up and the current camera/JPEG
preprocessing data path on that run. It does not establish long-term
reliability, user safety, production readiness, inference accuracy, or complete
assistive behavior.

See [the recorded preprocessing results](/testing/results) for the measured
image dimensions and throughput.
