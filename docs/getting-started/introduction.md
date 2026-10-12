# Introduction

Smart Cap is an open embedded-systems prototype exploring a camera-based
assistive device for people with visual impairments. The intended direction is
to interpret nearby scenes and communicate concise, useful information.

The current project is an early hardware and image-processing foundation, not
a finished wearable product. It currently initializes its board peripherals
and converts camera JPEG frames into a bounded, smaller pixel buffer.

## What works today

- ESP-IDF 5.5 firmware for an ESP32-S3 target.
- Camera initialization and latest-frame capture on the T-Camera S3 V1.6.
- JPEG decoding, image resizing, and RGB565, RGB888, or grayscale output.
- Optional I²C detection/initialization of the SSD1306 OLED and AXP2101 PMU.
- A recorded hardware preprocessing run: 270 processed images, zero reported
  processing failures, and approximately 9 processed images per second.

The measured throughput is the end-to-end image preprocessing rate for that
recorded run. It is **not** a separately measured sensor capture FPS and does
not establish long-term stability.

## What is planned, not implemented

Object recognition, distance or motion estimation, spoken guidance, and
navigation recommendations are not part of the current firmware. No claim is
made that these features work or have been tested.

## Continue

- [Check the current project status](/getting-started/status)
- [Prepare the development environment](/getting-started/environment)
- [Build, flash, and monitor](/getting-started/build-flash)
- [Read the architecture overview](/architecture/overview)
