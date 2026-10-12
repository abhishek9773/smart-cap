#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

esp_err_t i2c_bus_initialize(void);
esp_err_t i2c_bus_add_device(
    uint8_t address,
    i2c_master_dev_handle_t *device);
esp_err_t i2c_bus_remove_device(i2c_master_dev_handle_t *device);
esp_err_t i2c_bus_read_register(
    i2c_master_dev_handle_t device,
    uint8_t reg,
    uint8_t *value);
esp_err_t i2c_bus_write_register(
    i2c_master_dev_handle_t device,
    uint8_t reg,
    uint8_t value);
esp_err_t i2c_bus_deinitialize(void);
