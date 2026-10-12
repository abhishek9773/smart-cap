#pragma once

#include <stdbool.h>

#include "driver/i2c_master.h"

bool camera_test_run(
    i2c_master_dev_handle_t oled_device,
    bool oled_ready);
