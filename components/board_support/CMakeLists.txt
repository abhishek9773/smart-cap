idf_component_register(
    SRCS "board_support.c"
    INCLUDE_DIRS "include"
    REQUIRES axp2101 esp_driver_gpio esp_driver_i2c i2c_bus log
)
