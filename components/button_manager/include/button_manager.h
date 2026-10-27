#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t button_manager_init(void);
bool button_manager_mode_switch_available(void);
