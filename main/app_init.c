#include "app_init.h"

#include "audio_player.h"
#include "board_support.h"
#include "button_manager.h"
#include "camera_driver.h"
#include "event_manager.h"
#include "esp_log.h"
#include "haptics.h"
#include "http_api.h"
#include "sensor_manager.h"
#include "sdkconfig.h"
#include "system_status.h"
#include "wifi_manager.h"

static const char *TAG = "smartcap_init";

esp_err_t app_init(void)
{
    system_status_init();
    event_manager_init();
    event_manager_record(EVENT_DEVICE_BOOT);

    esp_err_t err = button_manager_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Physical mode switch unavailable: %s", esp_err_to_name(err));
    }

    err = sensor_manager_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Distance sensors unavailable: %s", esp_err_to_name(err));
    }

    err = haptics_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Haptic drivers unavailable: %s", esp_err_to_name(err));
    }

    err = audio_player_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Audio output unavailable: %s", esp_err_to_name(err));
    }

    const esp_err_t power_rails_err = board_support_configure_power_rails();
    if (power_rails_err != ESP_OK) {
        ESP_LOGW(TAG, "AXP2101 board power rails were not configured: %s",
                 esp_err_to_name(power_rails_err));
    }

    err = wifi_manager_start();
    if (err != ESP_OK) {
        system_status_set_last_error((uint32_t)err);
        ESP_LOGE(TAG, "Wi-Fi AP initialization failed: %s", esp_err_to_name(err));
    }

    const camera_driver_config_t camera_config = {
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 16,
        .framebuffer_count = 2,
    };
    err = camera_driver_init(&camera_config);
    if (err == ESP_OK) {
        system_status_set_camera_initialized(true);
        event_manager_record(EVENT_CAMERA_READY);
    } else {
        system_status_set_camera_initialized(false);
        system_status_set_last_error((uint32_t)err);
        ESP_LOGE(TAG, "Camera initialization failed: %s", esp_err_to_name(err));
    }

    err = http_api_start();
    if (err != ESP_OK) {
        system_status_set_last_error((uint32_t)err);
        ESP_LOGE(TAG, "HTTP API initialization failed: %s", esp_err_to_name(err));
        return err;
    }

#if CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE
    if (power_rails_err != ESP_OK) {
        ESP_LOGE(TAG, "Motor test not started because the AXP2101 board rails were not configured");
    } else {
        err = haptics_start_motor_test();
        if (err != ESP_OK) {
            system_status_set_last_error((uint32_t)err);
            ESP_LOGE(TAG, "Motor test could not start: %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGW(TAG, "Repeating motor hardware test is enabled; stop with POST /api/v1/command {\"command\":\"stop_haptics\"}");
    }
#endif

    ESP_LOGI(TAG, "Application initialization complete; see Wi-Fi menuconfig for the local API address");
    return ESP_OK;
}
