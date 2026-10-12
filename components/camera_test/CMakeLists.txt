idf_component_register(
    SRCS "camera_test.c"
    INCLUDE_DIRS "include"
    REQUIRES board_support esp32-camera esp_timer freertos ssd1306
)
