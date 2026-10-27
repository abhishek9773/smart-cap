idf_component_register(
    SRCS "system_status.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common esp_timer freertos heap
)
