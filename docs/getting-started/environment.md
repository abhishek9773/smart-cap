# Development environment

The firmware project uses **ESP-IDF 5.5** and targets `esp32s3`. Use an
ESP-IDF installation with its environment activated so `idf.py` is available.
The repository also includes an ESP-IDF VS Code dev-container configuration.

The ESP-IDF component manager resolves the camera driver and JPEG decoder
dependencies from the project manifests. The committed
[`dependencies.lock`](../../dependencies.lock) records the resolved component
versions.

The documentation site is a Node.js/VitePress project under `docs/`; it does
not configure or build the firmware.

## Check the target

From the firmware repository root:

```sh
idf.py --version
idf.py set-target esp32s3
```

The target-setting command selects the ESP32-S3 build target. It may regenerate
the firmware's local `sdkconfig`; do not use it when you only intend to build
the documentation site.

For Node.js and VitePress setup, see
[Development workflow](/development/workflow).
