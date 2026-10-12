#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "axp2101.h"
#include "board_config.h"
#include "camera_test.h"
#include "i2c_bus.h"
#include "ssd1306.h"

static const char *TAG = "SMART_CAP_HW_TEST";

static void initialize_optional_oled(
    i2c_master_dev_handle_t *oled_device,
    bool *oled_ready)
{
    esp_err_t err = i2c_bus_add_device(
        SSD1306_I2C_ADDRESS,
        oled_device);

    if (err == ESP_OK) {
        err = ssd1306_initialize(*oled_device);
    }

    if (err == ESP_OK) {
        err = ssd1306_show_test_pattern(*oled_device, 0xAA);
    }

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "OLED initialization/test failed: %s",
            esp_err_to_name(err));

        const esp_err_t remove_err = i2c_bus_remove_device(oled_device);
        if (remove_err != ESP_OK) {
            ESP_LOGE(
                TAG,
                "OLED cleanup failed: %s",
                esp_err_to_name(remove_err));
        }
        return;
    }

    *oled_ready = true;
    ESP_LOGI(
        TAG,
        "OLED responded at address 0x%02x",
        SSD1306_I2C_ADDRESS);
}

static void check_optional_pmu(void)
{
    i2c_master_dev_handle_t pmu_device = NULL;
    esp_err_t err = i2c_bus_add_device(
        AXP2101_I2C_ADDRESS,
        &pmu_device);

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "PMU registration failed: %s",
            esp_err_to_name(err));
        return;
    }

    uint8_t chip_id = 0;
    err = axp2101_read_chip_id(pmu_device, &chip_id);

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "PMU chip-ID read failed: %s",
            esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "PMU chip ID register: 0x%02x", chip_id);

        if (chip_id != AXP2101_CHIP_ID) {
            ESP_LOGW(
                TAG,
                "PMU ID differs from expected AXP2101 ID. "
                "Camera power rails will not be configured.");
        } else {
            err = axp2101_configure_camera_rails(pmu_device);
            if (err != ESP_OK) {
                ESP_LOGE(
                    TAG,
                    "Camera power-rail configuration failed: %s",
                    esp_err_to_name(err));
            } else {
                ESP_LOGI(
                    TAG,
                    "Camera rails enabled using LilyGO factory settings: "
                    "ALDO1=1500mV, ALDO2=3000mV, ALDO4=3000mV");
            }
        }
    }

    err = i2c_bus_remove_device(&pmu_device);
    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "PMU device cleanup failed: %s",
            esp_err_to_name(err));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "LILYGO T-Camera S3 V1.6 hardware test");

    const esp_err_t i2c_err = i2c_bus_initialize();
    if (i2c_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "I2C initialization failed: %s",
            esp_err_to_name(i2c_err));

        if (!camera_test_run(NULL, false)) {
            ESP_LOGE(TAG, "Camera capture test failed");
        }
        return;
    }

    i2c_master_dev_handle_t oled_device = NULL;
    bool oled_ready = false;
    initialize_optional_oled(&oled_device, &oled_ready);
    check_optional_pmu();

    ESP_LOGI(
        TAG,
        "Starting camera test; camera rails were configured if AXP2101 was identified.");

    const bool camera_test_ok = camera_test_run(oled_device, oled_ready);
    if (!camera_test_ok) {
        ESP_LOGE(TAG, "Camera capture test failed");
    }

    esp_err_t cleanup_err = i2c_bus_remove_device(&oled_device);
    if (cleanup_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "OLED cleanup failed: %s",
            esp_err_to_name(cleanup_err));
    }

    cleanup_err = i2c_bus_deinitialize();
    if (cleanup_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "I2C bus cleanup failed: %s",
            esp_err_to_name(cleanup_err));
    }
}
