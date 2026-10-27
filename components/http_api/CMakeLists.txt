idf_component_register(
    SRCS "http_api.c"
    INCLUDE_DIRS "include"
    REQUIRES audio_player button_manager camera_driver event_manager esp_http_server esp_timer haptics json log sensor_manager system_status wifi_manager
)
