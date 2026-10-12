#include "camera_pipeline.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct camera_pipeline {
    image_preprocessor_handle_t preprocessor;
    bool initialized;
};

esp_err_t camera_pipeline_create(
    const camera_pipeline_config_t *config,
    camera_pipeline_handle_t *pipeline)
{
    if (config == NULL || pipeline == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *pipeline = NULL;

    camera_pipeline_handle_t instance = calloc(1, sizeof(*instance));
    if (instance == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = image_preprocessor_create(
        &config->image,
        &instance->preprocessor);
    if (err != ESP_OK) {
        free(instance);
        return err;
    }

    err = camera_driver_init(&config->camera);
    if (err != ESP_OK) {
        image_preprocessor_destroy(instance->preprocessor);
        free(instance);
        return err;
    }
    instance->initialized = true;

    *pipeline = instance;
    return ESP_OK;
}

esp_err_t camera_pipeline_process_next(
    camera_pipeline_handle_t pipeline,
    image_preprocessed_image_t *image)
{
    if (pipeline == NULL || image == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(image, 0, sizeof(*image));
    if (!pipeline->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    camera_fb_t *frame = NULL;
    esp_err_t err = camera_driver_acquire(&frame);
    if (err != ESP_OK) {
        return err;
    }

    if (frame->format != PIXFORMAT_JPEG) {
        err = ESP_ERR_NOT_SUPPORTED;
    } else {
        err = image_preprocessor_stage_jpeg(
            pipeline->preprocessor,
            frame->buf,
            frame->len);
    }

    const esp_err_t release_err = camera_driver_release(frame);
    if (release_err != ESP_OK) {
        image_preprocessor_cancel_staged(pipeline->preprocessor);
        return release_err;
    }
    if (err != ESP_OK) {
        return err;
    }

    return image_preprocessor_process_staged(pipeline->preprocessor, image);
}

esp_err_t camera_pipeline_destroy(camera_pipeline_handle_t pipeline)
{
    if (pipeline == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (pipeline->initialized) {
        const esp_err_t err = camera_driver_deinit();
        if (err != ESP_OK) {
            return err;
        }
    }

    image_preprocessor_destroy(pipeline->preprocessor);
    free(pipeline);
    return ESP_OK;
}
