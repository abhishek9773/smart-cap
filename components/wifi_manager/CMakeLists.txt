idf_component_register(
    SRCS "wifi_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_netif esp_wifi event_manager lwip nvs_flash
)
