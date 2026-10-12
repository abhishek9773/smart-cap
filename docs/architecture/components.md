# Project components

| Path | Responsibility |
| --- | --- |
| `components/board_support/` | Shared T-Camera S3 V1.6 GPIO, I²C, PMU, and OLED constants |
| `components/camera_driver/` | Builds the `esp32-camera` configuration and wraps init, acquire, release, and deinit |
| `components/camera_pipeline/` | Orchestrates capture, JPEG staging, prompt buffer release, and preprocessing |
| `components/image_preprocessor/` | Owns bounded reusable memory; decodes JPEG, scales, resizes, and converts pixels |
| `components/axp2101/` | Reads the PMU ID and configures the existing camera rail preset |
| `components/i2c_bus/` | Initializes and manages the shared board I²C bus |
| `components/ssd1306/` | Initializes the optional OLED and writes a test pattern |
| `main/main.c` | Startup ordering, optional peripherals, and the 30-second preprocessing validation |

## Initialization and processing

`main/main.c` initializes I²C and probes the optional OLED and PMU. On a
matching AXP2101 ID it applies the existing camera rail settings. It then
creates the pipeline with QVGA JPEG and image-output limits. The pipeline
allocates preprocessing buffers and initializes the camera. Each processing
request captures a frame, copies its JPEG bytes, releases the camera buffer,
then decodes and creates the configured output image.

## Boundaries

The components currently handle image transport and preprocessing only. There
is no inference component, audio output component, distance estimator, or
navigation planner in the application path.
