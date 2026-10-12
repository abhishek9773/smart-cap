idf_component_register(
    SRCS "ssd1306.c"
    INCLUDE_DIRS "include"
    REQUIRES board_support esp_driver_i2c
)
