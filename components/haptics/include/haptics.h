#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

#define HAPTICS_LEFT_GPIO GPIO_NUM_15
#define HAPTICS_RIGHT_GPIO GPIO_NUM_16

typedef struct {
    bool configured;
    bool left_active;
    bool right_active;
    const char *pattern;
} haptics_status_t;

esp_err_t haptics_init(void);
esp_err_t haptics_start_motor_test(void);
void haptics_get_status(haptics_status_t *status);
esp_err_t haptics_start_pattern(const char *pattern);
esp_err_t haptics_stop(void);
