#include "i2c_bus.h"

#include "board_config.h"

static i2c_master_bus_handle_t s_i2c_bus;

esp_err_t i2c_bus_initialize(void)
{
    const i2c_master_bus_config_t config = {
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA,
        .scl_io_num = BOARD_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&config, &s_i2c_bus);
}

esp_err_t i2c_bus_add_device(
    uint8_t address,
    i2c_master_dev_handle_t *device)
{
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = BOARD_I2C_FREQUENCY_HZ,
    };

    return i2c_master_bus_add_device(s_i2c_bus, &config, device);
}

esp_err_t i2c_bus_remove_device(i2c_master_dev_handle_t *device)
{
    if (device == NULL || *device == NULL) {
        return ESP_OK;
    }

    const esp_err_t err = i2c_master_bus_rm_device(*device);
    if (err == ESP_OK) {
        *device = NULL;
    }
    return err;
}

esp_err_t i2c_bus_read_register(
    i2c_master_dev_handle_t device,
    uint8_t reg,
    uint8_t *value)
{
    return i2c_master_transmit_receive(
        device,
        &reg,
        sizeof(reg),
        value,
        sizeof(*value),
        BOARD_I2C_TIMEOUT_MS);
}

esp_err_t i2c_bus_write_register(
    i2c_master_dev_handle_t device,
    uint8_t reg,
    uint8_t value)
{
    const uint8_t data[] = {reg, value};

    return i2c_master_transmit(
        device,
        data,
        sizeof(data),
        BOARD_I2C_TIMEOUT_MS);
}

esp_err_t i2c_bus_deinitialize(void)
{
    if (s_i2c_bus == NULL) {
        return ESP_OK;
    }

    const esp_err_t err = i2c_del_master_bus(s_i2c_bus);
    if (err == ESP_OK) {
        s_i2c_bus = NULL;
    }
    return err;
}
