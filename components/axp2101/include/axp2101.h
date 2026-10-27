#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

esp_err_t axp2101_read_chip_id(
    i2c_master_dev_handle_t device,
    uint8_t *chip_id);
esp_err_t axp2101_configure_board_rails(
    i2c_master_dev_handle_t device);
