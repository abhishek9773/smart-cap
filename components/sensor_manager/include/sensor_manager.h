#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    bool distance_sensors_available;
    bool left_valid;
    bool right_valid;
    int left_mm;
    int right_mm;
    bool battery_measurement_available;
    bool battery_percentage_valid;
    bool charging_status_available;
    int battery_voltage_mv;
    int battery_percentage;
    bool charging;
} sensor_manager_status_t;

esp_err_t sensor_manager_init(void);
void sensor_manager_get_status(sensor_manager_status_t *status);
