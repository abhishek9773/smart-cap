#pragma once

#include "esp_camera.h"
#include "esp_err.h"

typedef struct {
    framesize_t frame_size;
    uint8_t jpeg_quality;
    size_t framebuffer_count;
} camera_driver_config_t;

esp_err_t camera_driver_init(const camera_driver_config_t *config);
esp_err_t camera_driver_acquire(camera_fb_t **frame);
esp_err_t camera_driver_release(camera_fb_t *frame);
esp_err_t camera_driver_deinit(void);
