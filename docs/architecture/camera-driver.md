# Camera driver

The reusable driver lives in `components/camera_driver/` and wraps Espressif's
`esp32-camera` component. It takes frame size, JPEG quality, and framebuffer
count as configuration. The active validation selects QVGA, quality 12, and
two framebuffers.

The board mapping and capture policy are kept in the driver configuration:

- OV2640 output is JPEG.
- XCLK is configured at 20 MHz.
- Framebuffers are allocated in PSRAM.
- Capture uses `CAMERA_GRAB_LATEST`.
- Application calls acquire/release to make ownership explicit.

The driver uses the verified board pin constants in
[`board_config.h`](https://github.com/abhishek9773/smart-cap/blob/main/components/board_support/include/board_config.h).
Refer to [the exact mapping](/hardware/pins) before making board-specific
changes.

## Frame ownership

`camera_driver_acquire()` obtains a camera-owned `camera_fb_t`. The caller must
release it with `camera_driver_release()`. The pipeline copies the frame bytes
to its staging buffer and returns the camera buffer before JPEG decoding. This
reduces the time a capture buffer remains held by application processing.

Latest-frame mode is the camera driver's policy for limiting stale frame
backlog; the application does not build a queue or run on a fixed capture
timer.
