#include "app_init.h"

#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "smartcap";

void app_main(void)
{
    const esp_err_t err = app_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Application initialization failed: %s", esp_err_to_name(err));
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
