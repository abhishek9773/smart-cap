#include "board_support.h"

#include "axp2101.h"
#include "board_config.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "i2c_bus.h"

static const char *TAG = "board_support";

esp_err_t board_support_configure_power_rails(void)
{
    esp_err_t err = i2c_bus_initialize();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PMU I2C bus: %s", esp_err_to_name(err));
        return err;
    }

    i2c_master_dev_handle_t pmu = NULL;
    err = i2c_bus_add_device(AXP2101_I2C_ADDRESS, &pmu);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add AXP2101 at I2C address 0x%02X: %s",
                 AXP2101_I2C_ADDRESS, esp_err_to_name(err));
    } else {
        uint8_t chip_id = 0;
        err = axp2101_read_chip_id(pmu, &chip_id);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read AXP2101 chip ID: %s", esp_err_to_name(err));
        } else if (chip_id != AXP2101_CHIP_ID) {
            ESP_LOGE(TAG, "Unexpected PMU chip ID 0x%02X (expected 0x%02X)",
                     chip_id, AXP2101_CHIP_ID);
            err = ESP_ERR_NOT_SUPPORTED;
        } else {
            ESP_LOGI(TAG, "AXP2101 detected; configuring expansion 3.3V and camera rails");
            err = axp2101_configure_board_rails(pmu);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure AXP2101 power rails: %s",
                         esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "AXP2101 DCDC3 expansion output configured to %u mV",
                         BOARD_EXTERNAL_3V3_MILLIVOLTS);
            }
        }

        const esp_err_t remove_err = i2c_bus_remove_device(&pmu);
        if (err == ESP_OK && remove_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove AXP2101 I2C device: %s",
                     esp_err_to_name(remove_err));
            err = remove_err;
        }
    }

    const esp_err_t deinit_err = i2c_bus_deinitialize();
    if (err == ESP_OK && deinit_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize PMU I2C bus: %s",
                 esp_err_to_name(deinit_err));
        err = deinit_err;
    }
    return err;
}
