#pragma once

#include "camera_driver.h"
#include "esp_err.h"
#include "image_preprocessor.h"

typedef struct camera_pipeline *camera_pipeline_handle_t;

typedef struct {
    camera_driver_config_t camera;
    image_preprocessor_config_t image;
} camera_pipeline_config_t;

esp_err_t camera_pipeline_create(
    const camera_pipeline_config_t *config,
    camera_pipeline_handle_t *pipeline);
esp_err_t camera_pipeline_process_next(
    camera_pipeline_handle_t pipeline,
    image_preprocessed_image_t *image);
esp_err_t camera_pipeline_destroy(camera_pipeline_handle_t pipeline);
