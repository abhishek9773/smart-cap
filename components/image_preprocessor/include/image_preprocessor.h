#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct image_preprocessor *image_preprocessor_handle_t;

typedef enum {
    IMAGE_PIXEL_FORMAT_RGB888,
    IMAGE_PIXEL_FORMAT_RGB565,
    IMAGE_PIXEL_FORMAT_GRAY8,
} image_pixel_format_t;

typedef struct {
    size_t max_jpeg_bytes;
    size_t max_decode_pixels;
    size_t max_output_pixels;
    uint16_t output_width;
    uint16_t output_height;
    image_pixel_format_t output_format;
} image_preprocessor_config_t;

typedef struct {
    const uint8_t *data;
    size_t len;
    size_t stride;
    uint16_t width;
    uint16_t height;
    uint16_t source_width;
    uint16_t source_height;
    image_pixel_format_t format;
} image_preprocessed_image_t;

/* The output data is owned by the preprocessor and reused by later calls. */
esp_err_t image_preprocessor_create(
    const image_preprocessor_config_t *config,
    image_preprocessor_handle_t *preprocessor);
void image_preprocessor_destroy(image_preprocessor_handle_t preprocessor);
esp_err_t image_preprocessor_stage_jpeg(
    image_preprocessor_handle_t preprocessor,
    const uint8_t *jpeg_data,
    size_t jpeg_len);
esp_err_t image_preprocessor_process_staged(
    image_preprocessor_handle_t preprocessor,
    image_preprocessed_image_t *image);
void image_preprocessor_cancel_staged(
    image_preprocessor_handle_t preprocessor);
