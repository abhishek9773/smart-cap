# Camera preprocessing validation

This procedure requires a LILYGO T-Camera S3 V1.6 with the OV2640 camera,
SSD1306 display, and AXP2101 PMU connected.

1. Build and flash the ESP-IDF 5.5 project:

   ```sh
   idf.py set-target esp32s3
   idf.py build
   idf.py -p /dev/ttyACM0 flash
   ```

2. Open the serial monitor at 115200 baud:

   ```sh
   idf.py -p /dev/ttyACM0 monitor
   ```

3. Confirm the OLED responds, the PMU reports chip ID `0x4a`, and the
   verified camera rails are configured.
4. Confirm the driver detects the OV2640 and completes camera configuration.
5. Let the preprocessing validation run to its final summary. It runs for at
   least 30 seconds, decodes each JPEG to a configured size/format, returns
   each camera-owned frame buffer before decoding, and reports processed and
   failed images plus average output size.

## Expected successful log sequence

Exact timings and processed-frame counts depend on the board and scene. The
important success indicators are a detected OV2640, a first-image message,
and a final summary with at least one successfully preprocessed frame:

```text
SMART_CAP_HW_TEST: OLED responded at address 0x3c
SMART_CAP_HW_TEST: PMU chip ID register: 0x4a
SMART_CAP_HW_TEST: Camera rails enabled using LilyGO factory settings: ALDO1=1500mV, ALDO2=3000mV, ALDO4=3000mV
camera: Detected OV2640 camera
camera: cam config ok
SMART_CAP_HW_TEST: Preprocessing test started: QVGA JPEG -> 160x120 RGB565; 30-second demand-driven run, latest-frame capture, no timer
SMART_CAP_HW_TEST: First preprocessed image: input=320x240, output=160x120, format=RGB565, length=38400 bytes
SMART_CAP_HW_TEST: Preprocessing test complete: duration=...ms | ... processed | ... failed | average output=38400 bytes
```

This procedure exercises the real PMU, camera, PSRAM framebuffer allocation,
DVP capture path, JPEG decoder, resizing, and RGB565 conversion. It cannot be
substituted with a host-only unit test. Record and review the serial output for
each physical board revision.

## Failure evidence

If initialization succeeds but the final processed-frame count is zero, save
the full boot and monitor log. Sensor detection alone is not proof of DVP/VSYNC
frame delivery. Decode, input-bound, and allocation errors are reported by the
preprocessing validation and should be distinguished from camera capture
timeouts.
