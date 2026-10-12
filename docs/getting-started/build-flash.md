# Build, flash, and monitor

Run firmware commands from the repository root in an ESP-IDF 5.5 environment.
These commands operate on the firmware; the documentation site has separate
commands described in [Development workflow](/development/workflow).

## Build

```sh
idf.py set-target esp32s3
idf.py build
```

`set-target` selects the ESP32-S3 target. `build` configures and compiles the
ESP-IDF application and its managed components.

## Flash the connected board

```sh
idf.py -p /dev/ttyACM0 flash
```

Replace `/dev/ttyACM0` with the serial port assigned to the board.

## Read runtime logs

```sh
idf.py -p /dev/ttyACM0 monitor
```

This opens the ESP-IDF serial monitor for the already-running firmware. Exit
the monitor with `Ctrl+]`.

To flash and immediately open the monitor in one command:

```sh
idf.py -p /dev/ttyACM0 flash monitor
```

Successful compilation or flashing alone does not establish that the camera
captured or preprocessed images. Compare the serial output with
[Hardware validation](/hardware/validation) and the
[recorded run](/testing/results).
