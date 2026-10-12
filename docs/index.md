---
layout: home

hero:
  name: PathSee
  text: Vision beyond barriers
  tagline: An assistive-technology project taking shape—starting with a real-time embedded camera and image-processing foundation.
  image:
    src: /hero-pipeline.svg
    alt: "Illustration of the PathSee camera pipeline: JPEG capture, image preprocessing, and a compact RGB565 output"
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

## Building toward greater independence

PathSee is an assistive-device research and engineering project intended to
help people with visual impairments better understand nearby surroundings. Its
long-term vision includes recognizing objects and communicating concise
guidance. **That complete assistive system is not yet finished.**

<div class="home-status">
  <span class="home-status-dot" aria-hidden="true"></span>
  <span><strong>Today:</strong> camera hardware + image preprocessing</span>
  <span class="home-status-divider" aria-hidden="true"></span>
  <span><strong>Next:</strong> assistive intelligence is future work</span>
</div>

### A working foundation

The current firmware boots on the **LILYGO T-Camera S3 V1.6**, initializes the
camera and optional board peripherals, captures OV2640 JPEG frames on demand,
and preprocesses each image into a smaller RGB565 buffer. A recorded
approximately 30-second hardware run completed 270 images with zero reported
preprocessing failures.

The complete assistive system is still under development. AI inference, object
detection, distance estimation, speech or voice alerts, and navigation
guidance are **not implemented or validated**.

| Start here | Learn about |
| --- | --- |
| [Introduction](/getting-started/introduction) | [System architecture](/architecture/overview) |
| [Build and flash](/getting-started/build-flash) | [Board and pin map](/hardware/board) |
| [Recorded results](/testing/results) | [Current limitations](/testing/limitations) |
