#include "axp2101.h"

#include "board_config.h"
#include "i2c_bus.h"

#define AXP2101_LDO_ONOFF_CTRL0 0x90
#define AXP2101_LDO_VOL0_CTRL 0x92
#define AXP2101_LDO_VOL1_CTRL 0x93
#define AXP2101_LDO_VOL3_CTRL 0x95

#define AXP2101_ALDO1_ENABLE_BIT 0
#define AXP2101_ALDO2_ENABLE_BIT 1
#define AXP2101_ALDO4_ENABLE_BIT 3

#define AXP2101_ALDO_VOLTAGE_MIN_MV 500
#define AXP2101_ALDO_VOLTAGE_MAX_MV 3500
#define AXP2101_ALDO_VOLTAGE_STEP_MV 100
#define AXP2101_ALDO_VOLTAGE_MASK 0x1F
#define AXP2101_ALDO_VOLTAGE_RESERVED_MASK 0xE0

/* Register layout and voltage encoding follow LilyGO's bundled XPowersLib. */
esp_err_t axp2101_read_chip_id(
    i2c_master_dev_handle_t device,
    uint8_t *chip_id)
{
    return i2c_bus_read_register(
        device,
        AXP2101_REG_CHIP_ID,
        chip_id);
}

static esp_err_t set_aldo_voltage(
    i2c_master_dev_handle_t device,
    uint8_t reg,
    uint16_t millivolts)
{
    if (millivolts < AXP2101_ALDO_VOLTAGE_MIN_MV ||
        millivolts > AXP2101_ALDO_VOLTAGE_MAX_MV ||
        millivolts % AXP2101_ALDO_VOLTAGE_STEP_MV != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t current = 0;
    esp_err_t err = i2c_bus_read_register(device, reg, &current);
    if (err != ESP_OK) {
        return err;
    }

    const uint8_t voltage_code = (uint8_t)(
        (millivolts - AXP2101_ALDO_VOLTAGE_MIN_MV) /
        AXP2101_ALDO_VOLTAGE_STEP_MV);
    const uint8_t updated =
        (current & AXP2101_ALDO_VOLTAGE_RESERVED_MASK) |
        (voltage_code & AXP2101_ALDO_VOLTAGE_MASK);

    err = i2c_bus_write_register(device, reg, updated);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t readback = 0;
    err = i2c_bus_read_register(device, reg, &readback);
    if (err != ESP_OK) {
        return err;
    }

    return (readback & AXP2101_ALDO_VOLTAGE_MASK) == voltage_code
        ? ESP_OK
        : ESP_FAIL;
}

static esp_err_t enable_aldo(
    i2c_master_dev_handle_t device,
    uint8_t enable_bit)
{
    uint8_t current = 0;
    esp_err_t err = i2c_bus_read_register(
        device,
        AXP2101_LDO_ONOFF_CTRL0,
        &current);
    if (err != ESP_OK) {
        return err;
    }

    const uint8_t updated = current | (uint8_t)(1U << enable_bit);
    err = i2c_bus_write_register(
        device,
        AXP2101_LDO_ONOFF_CTRL0,
        updated);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t readback = 0;
    err = i2c_bus_read_register(
        device,
        AXP2101_LDO_ONOFF_CTRL0,
        &readback);
    if (err != ESP_OK) {
        return err;
    }

    return (readback & (uint8_t)(1U << enable_bit)) != 0
        ? ESP_OK
        : ESP_FAIL;
}

esp_err_t axp2101_configure_camera_rails(
    i2c_master_dev_handle_t device)
{
    /* Match examples/factory/power.cpp; touch no other PMU outputs. */
    esp_err_t err = set_aldo_voltage(
        device, AXP2101_LDO_VOL0_CTRL, 1500);
    if (err != ESP_OK) {
        return err;
    }

    err = set_aldo_voltage(device, AXP2101_LDO_VOL1_CTRL, 3000);
    if (err != ESP_OK) {
        return err;
    }

    err = set_aldo_voltage(device, AXP2101_LDO_VOL3_CTRL, 3000);
    if (err != ESP_OK) {
        return err;
    }

    err = enable_aldo(device, AXP2101_ALDO1_ENABLE_BIT);
    if (err != ESP_OK) {
        return err;
    }

    err = enable_aldo(device, AXP2101_ALDO2_ENABLE_BIT);
    if (err != ESP_OK) {
        return err;
    }

    return enable_aldo(device, AXP2101_ALDO4_ENABLE_BIT);
}
