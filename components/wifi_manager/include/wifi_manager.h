#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    bool active;
    int connected_clients;
    char ssid[33];
    char ip_address[16];
} wifi_manager_status_t;

esp_err_t wifi_manager_start(void);
void wifi_manager_get_status(wifi_manager_status_t *status);
