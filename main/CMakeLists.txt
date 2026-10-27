idf_component_register(
    SRCS "app_main.c" "app_init.c"
    INCLUDE_DIRS "."
    REQUIRES
        board_support
        http_api
        audio_player
        button_manager
        camera_driver
        event_manager
        freertos
        haptics
        log
        sensor_manager
        system_status
        wifi_manager
)
