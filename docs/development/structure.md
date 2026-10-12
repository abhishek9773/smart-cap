# Project structure

```text
sample_project/
├── components/
│   ├── board_support/       Board pin and peripheral constants
│   ├── camera_driver/       Camera initialization and frame ownership
│   ├── camera_pipeline/     Demand-driven capture/preprocessing orchestration
│   ├── image_preprocessor/  Bounded JPEG decode, resize, and pixel conversion
│   ├── axp2101/             PMU access and camera-rail setup
│   ├── i2c_bus/             Shared I²C lifecycle
│   └── ssd1306/             Optional OLED initialization/test pattern
├── docs/
│   └── hardware-test.md     Existing hardware-validation procedure
├── main/
│   └── main.c               Firmware startup and preprocessing validation
└── docs/                    VitePress website and canonical project documentation
```

The VitePress project has its own `package.json`, lockfile, configuration,
theme, and Markdown pages inside `docs/`. Its dependency installation and build
do not require an ESP-IDF build and do not participate in the firmware CMake
project.

See [Project components](/architecture/components) for component
responsibilities.
