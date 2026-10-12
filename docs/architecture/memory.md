# Memory management

The preprocessor allocates fixed-capacity working buffers when it is created,
then reuses them for each image. Its configuration bounds JPEG input size,
decoded pixels, and output pixels. The current validation limits include:

| Buffer | Configured capacity |
| --- | ---: |
| Staged JPEG | 128 KiB |
| RGB888 decode intermediate | 320 × 240 × 3 = 230,400 bytes |
| RGB565 output | 160 × 120 × 2 = 38,400 bytes |
| JPEG decoder workspace | 3,100 bytes with this build's decoder setting |

These preprocessor allocations request PSRAM-capable memory. The camera
driver's two capture framebuffers are also configured for PSRAM. These values
describe the current validation setup, not a universal minimum or a guarantee
of available memory at runtime.

The JPEG decoder workspace size depends on `CONFIG_JD_FASTDECODE`; the
documented 3,100-byte value is the current project's setting. The implementation
uses 65,472 bytes when the setting equals 2.

The JPEG frame buffer is owned by the camera component. The pipeline copies
the captured bytes into its staging buffer and returns the camera buffer
before decoding, so the decoder does not retain a camera-owned frame.
