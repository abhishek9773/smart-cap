#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

esp_err_t ssd1306_initialize(i2c_master_dev_handle_t device);
esp_err_t ssd1306_show_test_pattern(
    i2c_master_dev_handle_t device,
    uint8_t pattern);
