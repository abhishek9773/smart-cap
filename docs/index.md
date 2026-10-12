---
layout: home

hero:
  name: Smart Cap
  text: Assistive vision, built incrementally
  tagline: An open embedded-systems prototype exploring camera-based environmental awareness for people with visual impairments.
  actions:
    - theme: brand
      text: Explore the guide
      link: /getting-started/introduction
    - theme: alt
      text: View recorded results
      link: /testing/results

features:
  - title: A real embedded platform
    details: ESP32-S3 firmware on the LILYGO T-Camera S3 V1.6, with OV2640 camera, PSRAM, optional OLED, and PMU.
  - title: Working image pipeline
    details: Demand-driven JPEG capture, bounded image buffers, JPEG decode, resize, and RGB565 output.
  - title: Honest project status
    details: Hardware and preprocessing validation are documented separately from the planned assistive and AI features.
---

## What is Smart Cap?

Smart Cap is an assistive-device research and engineering project intended to
help people with visual impairments better understand nearby surroundings. Its
long-term vision includes recognizing objects and communicating concise
guidance. **That complete assistive system is not yet finished.**

### Current implementation

The current firmware boots on the **LILYGO T-Camera S3 V1.6**, initializes the
camera and optional board peripherals, captures OV2640 JPEG frames on demand,
and preprocesses each image into a smaller RGB565 buffer. A recorded
approximately 30-second hardware run completed 270 images with zero reported
preprocessing failures.

### Not implemented

There is no AI inference, object detection, distance estimation, speech or
voice-alert output, or end-to-end navigation guidance in the current
implementation. These remain future work and have not been validated.

| Start here | Learn about |
| --- | --- |
| [Introduction](/getting-started/introduction) | [System architecture](/architecture/overview) |
| [Build and flash](/getting-started/build-flash) | [Board and pin map](/hardware/board) |
| [Recorded results](/testing/results) | [Current limitations](/testing/limitations) |
