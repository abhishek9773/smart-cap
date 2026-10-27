#include "sensor_manager.h"

#include <string.h>

esp_err_t sensor_manager_init(void)
{
    return ESP_OK;
}

void sensor_manager_get_status(sensor_manager_status_t *status)
{
    if (status != NULL) {
        memset(status, 0, sizeof(*status));
    }
}
