# Hardware test procedure

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
5. Let the capture test run to its final summary. It runs for at least
   30 seconds, returns each acquired frame buffer, and reports successful
   captures, timeouts, average JPEG size, and approximate FPS.

## Expected successful log sequence

Exact timestamps, frame sizes, and FPS depend on the board and scene. The
important success indicators are a detected OV2640, a first-frame message,
and a final summary with at least one successful frame:

```text
SMART_CAP_HW_TEST: OLED responded at address 0x3c
SMART_CAP_HW_TEST: PMU chip ID register: 0x4a
SMART_CAP_HW_TEST: Camera rails enabled using LilyGO factory settings: ALDO1=1500mV, ALDO2=3000mV, ALDO4=3000mV
camera: Detected OV2640 camera
camera: cam config ok
CAMERA_TEST: First frame: width=320, height=240, pixel_format=..., length=... bytes
CAMERA_TEST: CAMERA test complete: duration=...ms | ... FPS | ... successful | ... failed | avg JPEG ... bytes
```

This procedure exercises the real PMU, camera, PSRAM framebuffer allocation,
and DVP capture path. It cannot be substituted with a host-only unit test.
Record and review the serial output for each physical board revision.

## Failure evidence

If initialization succeeds but the final successful-frame count is zero,
save the full boot and monitor log. The camera component's
`esp_camera_fb_get()` waits for a complete frame-buffer queue entry and logs
`Failed to get frame: timeout` if none arrives. That indicates capture
completion did not reach the application; sensor detection alone is not
proof of DVP/VSYNC frame delivery.
