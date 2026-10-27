idf_component_register(
    SRCS "haptics.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common esp_driver_gpio freertos log
)
