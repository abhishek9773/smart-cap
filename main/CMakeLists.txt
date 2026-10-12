idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        axp2101
        board_support
        camera_test
        i2c_bus
        ssd1306
)