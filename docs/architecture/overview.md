# System overview

The firmware configures board peripherals, starts the camera and reusable
preprocessing pipeline, and repeatedly requests one image at a time while the
30-second validation is active.

![Camera image preprocessing flow: OV2640 JPEG capture, prompt frame-buffer release, bounded JPEG decode, resize and RGB565 conversion](/pipeline.svg)

The application also initializes the optional SSD1306 OLED and AXP2101 PMU over
the board's I²C bus. Those peripherals are not part of the image data path.

## Image data path

```text
OV2640 capture → JPEG frame → copy into bounded staging buffer
→ return camera frame buffer → JPEG decode/scale
→ nearest-neighbor resize and pixel conversion → RGB565 output
```

The camera driver's latest-frame mode avoids application-side frame queuing.
The captured JPEG is copied before releasing its camera-owned buffer; JPEG
decoding then runs against the pipeline's reusable staging and work buffers.
See [Camera pipeline](/architecture/camera-pipeline) and
[Memory management](/architecture/memory).

This firmware does not pass the image to an AI model or generate user-facing
guidance.
