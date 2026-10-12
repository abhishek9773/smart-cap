idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        axp2101
        board_support
        camera_pipeline
        i2c_bus
        esp_timer
        ssd1306
)