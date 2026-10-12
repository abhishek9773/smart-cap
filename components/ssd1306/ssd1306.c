#include "ssd1306.h"

#include <stddef.h>

#include "board_config.h"

static esp_err_t send_command(
    i2c_master_dev_handle_t device,
    uint8_t command)
{
    const uint8_t data[] = {0x00, command};

    return i2c_master_transmit(
        device, data, sizeof(data), BOARD_I2C_TIMEOUT_MS);
}

static esp_err_t send_command_parameter(
    i2c_master_dev_handle_t device,
    uint8_t command,
    uint8_t parameter)
{
    const uint8_t data[] = {0x00, command, parameter};

    return i2c_master_transmit(
        device, data, sizeof(data), BOARD_I2C_TIMEOUT_MS);
}

esp_err_t ssd1306_initialize(i2c_master_dev_handle_t device)
{
    static const uint8_t commands[][2] = {
        {0xAE, 0x00},
        {0xD5, 0x80},
        {0xA8, 0x3F},
        {0xD3, 0x00},
        {0x8D, 0x14},
        {0x20, 0x02},
        {0xA1, 0x00},
        {0xC8, 0x00},
        {0xDA, 0x12},
        {0x81, 0x7F},
        {0xD9, 0xF1},
        {0xDB, 0x40},
        {0xA4, 0x00},
        {0xA6, 0x00},
        {0xAF, 0x00},
    };

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        const uint8_t command = commands[i][0];
        esp_err_t err;

        if (command == 0xAE ||
            command == 0xA1 ||
            command == 0xC8 ||
            command == 0xA4 ||
            command == 0xA6 ||
            command == 0xAF) {
            err = send_command(device, command);
        } else {
            err = send_command_parameter(
                device,
                command,
                commands[i][1]);
        }

        if (err != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t ssd1306_show_test_pattern(
    i2c_master_dev_handle_t device,
    uint8_t pattern)
{
    uint8_t data[SSD1306_WIDTH + 1];
    data[0] = 0x40;

    for (size_t i = 1; i < sizeof(data); ++i) {
        data[i] = pattern;
    }

    for (uint8_t page = 0; page < SSD1306_PAGES; ++page) {
        const uint8_t page_commands[] = {
            0x00,
            (uint8_t)(0xB0U + page),
            0x00,
            0x10,
        };

        esp_err_t err = i2c_master_transmit(
            device,
            page_commands,
            sizeof(page_commands),
            BOARD_I2C_TIMEOUT_MS);
        if (err != ESP_OK) {
            return err;
        }

        err = i2c_master_transmit(
            device,
            data,
            sizeof(data),
            BOARD_I2C_TIMEOUT_MS);
        if (err != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}
