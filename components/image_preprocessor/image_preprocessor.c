#include "image_preprocessor.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "jpeg_decoder.h"
#include "sdkconfig.h"

#if CONFIG_JD_FASTDECODE == 2
#define JPEG_WORKSPACE_BYTES 65472U
#else
#define JPEG_WORKSPACE_BYTES 3100U
#endif

struct image_preprocessor {
    image_preprocessor_config_t config;
    uint8_t *jpeg_buffer;
    uint8_t *decoded_buffer;
    uint8_t *output_buffer;
    uint8_t *workspace;
    size_t jpeg_len;
    bool staged;
};

static bool multiply_size(size_t left, size_t right, size_t *result)
{
    if (right != 0 && left > SIZE_MAX / right) {
        return false;
    }
    *result = left * right;
    return true;
}

static size_t bytes_per_pixel(image_pixel_format_t format)
{
    switch (format) {
    case IMAGE_PIXEL_FORMAT_RGB888:
        return 3;
    case IMAGE_PIXEL_FORMAT_RGB565:
        return 2;
    case IMAGE_PIXEL_FORMAT_GRAY8:
        return 1;
    default:
        return 0;
    }
}

static void free_buffers(image_preprocessor_handle_t preprocessor)
{
    heap_caps_free(preprocessor->jpeg_buffer);
    heap_caps_free(preprocessor->decoded_buffer);
    heap_caps_free(preprocessor->output_buffer);
    heap_caps_free(preprocessor->workspace);
    free(preprocessor);
}

esp_err_t image_preprocessor_create(
    const image_preprocessor_config_t *config,
    image_preprocessor_handle_t *preprocessor)
{
    if (preprocessor == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *preprocessor = NULL;

    if (config == NULL || config->max_jpeg_bytes < 4 ||
        config->max_jpeg_bytes > UINT32_MAX ||
        config->max_decode_pixels == 0 ||
        config->max_output_pixels == 0 ||
        config->output_width == 0 ||
        config->output_height == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const size_t output_bpp = bytes_per_pixel(config->output_format);
    size_t configured_output_pixels = 0;
    size_t decoded_bytes = 0;
    size_t output_capacity = 0;
    if (output_bpp == 0 ||
        !multiply_size(
            config->output_width,
            config->output_height,
            &configured_output_pixels) ||
        configured_output_pixels > config->max_output_pixels ||
        !multiply_size(config->max_decode_pixels, 3, &decoded_bytes) ||
        decoded_bytes > UINT32_MAX ||
        !multiply_size(
            config->max_output_pixels,
            output_bpp,
            &output_capacity)) {
        return ESP_ERR_INVALID_ARG;
    }

    image_preprocessor_handle_t instance = calloc(1, sizeof(*instance));
    if (instance == NULL) {
        return ESP_ERR_NO_MEM;
    }
    instance->config = *config;
    instance->jpeg_buffer = heap_caps_malloc(
        config->max_jpeg_bytes,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    instance->decoded_buffer = heap_caps_malloc(
        decoded_bytes,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    instance->output_buffer = heap_caps_malloc(
        output_capacity,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    instance->workspace = heap_caps_malloc(
        JPEG_WORKSPACE_BYTES,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (instance->jpeg_buffer == NULL ||
        instance->decoded_buffer == NULL ||
        instance->output_buffer == NULL ||
        instance->workspace == NULL) {
        free_buffers(instance);
        return ESP_ERR_NO_MEM;
    }

    *preprocessor = instance;
    return ESP_OK;
}

void image_preprocessor_destroy(image_preprocessor_handle_t preprocessor)
{
    if (preprocessor != NULL) {
        free_buffers(preprocessor);
    }
}

esp_err_t image_preprocessor_stage_jpeg(
    image_preprocessor_handle_t preprocessor,
    const uint8_t *jpeg_data,
    size_t jpeg_len)
{
    if (preprocessor == NULL || jpeg_data == NULL || jpeg_len < 4) {
        return ESP_ERR_INVALID_ARG;
    }
    if (preprocessor->staged) {
        return ESP_ERR_INVALID_STATE;
    }
    if (jpeg_len > preprocessor->config.max_jpeg_bytes) {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(preprocessor->jpeg_buffer, jpeg_data, jpeg_len);
    preprocessor->jpeg_len = jpeg_len;
    preprocessor->staged = true;
    return ESP_OK;
}

static esp_err_t select_decode_scale(
    esp_jpeg_image_cfg_t *jpeg_config,
    size_t max_decode_pixels,
    uint16_t output_width,
    uint16_t output_height,
    esp_jpeg_image_scale_t *selected_scale,
    esp_jpeg_image_output_t *selected_info,
    esp_jpeg_image_output_t *source_info_out)
{
    static const esp_jpeg_image_scale_t scales[] = {
        JPEG_IMAGE_SCALE_1_8,
        JPEG_IMAGE_SCALE_1_4,
        JPEG_IMAGE_SCALE_1_2,
        JPEG_IMAGE_SCALE_0,
    };
    esp_jpeg_image_output_t source_info = {0};
    jpeg_config->out_scale = JPEG_IMAGE_SCALE_0;

    esp_err_t err = esp_jpeg_get_image_info(jpeg_config, &source_info);
    if (err != ESP_OK) {
        return err;
    }
    if (source_info.width == 0 || source_info.height == 0) {
        return ESP_ERR_INVALID_SIZE;
    }
    *source_info_out = source_info;

    bool found_with_no_upscale = false;
    esp_jpeg_image_output_t fallback_info = {0};
    esp_jpeg_image_scale_t fallback_scale = JPEG_IMAGE_SCALE_0;
    for (size_t index = 0; index < sizeof(scales) / sizeof(scales[0]); ++index) {
        const esp_jpeg_image_scale_t scale = scales[index];
        const uint32_t divisor = 1U << (uint32_t)scale;
        const uint16_t width = source_info.width / divisor;
        const uint16_t height = source_info.height / divisor;
        const uint64_t pixels = (uint64_t)width * height;

        if (width == 0 || height == 0 || pixels > max_decode_pixels) {
            continue;
        }

        fallback_info.width = width;
        fallback_info.height = height;
        fallback_scale = scale;

        if (width >= output_width && height >= output_height) {
            *selected_scale = scale;
            selected_info->width = width;
            selected_info->height = height;
            found_with_no_upscale = true;
            break;
        }
    }

    if (found_with_no_upscale) {
        return ESP_OK;
    }
    if (fallback_info.width == 0 || fallback_info.height == 0) {
        return ESP_ERR_INVALID_SIZE;
    }

    *selected_scale = fallback_scale;
    *selected_info = fallback_info;
    return ESP_OK;
}

static void convert_and_resize(
    const image_preprocessor_handle_t preprocessor,
    const esp_jpeg_image_output_t *decoded_info,
    const esp_jpeg_image_output_t *source_info,
    image_preprocessed_image_t *image)
{
    const size_t output_bpp = bytes_per_pixel(preprocessor->config.output_format);
    const size_t source_width = decoded_info->width;
    const size_t source_height = decoded_info->height;
    const size_t output_width = preprocessor->config.output_width;
    const size_t output_height = preprocessor->config.output_height;

    for (size_t y = 0; y < output_height; ++y) {
        const size_t source_y = (y * source_height) / output_height;
        for (size_t x = 0; x < output_width; ++x) {
            const size_t source_x = (x * source_width) / output_width;
            const uint8_t *source = preprocessor->decoded_buffer +
                (source_y * source_width + source_x) * 3;
            uint8_t *destination = preprocessor->output_buffer +
                (y * output_width + x) * output_bpp;

            switch (preprocessor->config.output_format) {
            case IMAGE_PIXEL_FORMAT_RGB888:
                destination[0] = source[0];
                destination[1] = source[1];
                destination[2] = source[2];
                break;
            case IMAGE_PIXEL_FORMAT_RGB565: {
                const uint16_t rgb565 = (uint16_t)(
                    ((uint16_t)(source[0] & 0xF8U) << 8) |
                    ((uint16_t)(source[1] & 0xFCU) << 3) |
                    (source[2] >> 3));
                destination[0] = (uint8_t)rgb565;
                destination[1] = (uint8_t)(rgb565 >> 8);
                break;
            }
            case IMAGE_PIXEL_FORMAT_GRAY8:
                destination[0] = (uint8_t)(
                    (77U * source[0] + 150U * source[1] +
                     29U * source[2]) >> 8);
                break;
            }
        }
    }

    image->data = preprocessor->output_buffer;
    image->len = output_width * output_height * output_bpp;
    image->stride = output_width * output_bpp;
    image->width = preprocessor->config.output_width;
    image->height = preprocessor->config.output_height;
    image->source_width = source_info->width;
    image->source_height = source_info->height;
    image->format = preprocessor->config.output_format;
}

esp_err_t image_preprocessor_process_staged(
    image_preprocessor_handle_t preprocessor,
    image_preprocessed_image_t *image)
{
    if (preprocessor == NULL || image == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(image, 0, sizeof(*image));
    if (!preprocessor->staged) {
        return ESP_ERR_INVALID_STATE;
    }
    preprocessor->staged = false;

    esp_jpeg_image_cfg_t jpeg_config = {
        .indata = preprocessor->jpeg_buffer,
        .indata_size = (uint32_t)preprocessor->jpeg_len,
        .outbuf = preprocessor->decoded_buffer,
        .outbuf_size = (uint32_t)(
            preprocessor->config.max_decode_pixels * 3U),
        .out_format = JPEG_IMAGE_FORMAT_RGB888,
        .advanced = {
            .working_buffer = preprocessor->workspace,
            .working_buffer_size = JPEG_WORKSPACE_BYTES,
        },
    };
    esp_jpeg_image_scale_t scale = JPEG_IMAGE_SCALE_0;
    esp_jpeg_image_output_t expected_info = {0};
    esp_jpeg_image_output_t source_info = {0};
    esp_err_t err = select_decode_scale(
        &jpeg_config,
        preprocessor->config.max_decode_pixels,
        preprocessor->config.output_width,
        preprocessor->config.output_height,
        &scale,
        &expected_info,
        &source_info);
    if (err != ESP_OK) {
        return err;
    }
    jpeg_config.out_scale = scale;

    esp_jpeg_image_output_t decoded_info = {0};
    err = esp_jpeg_decode(&jpeg_config, &decoded_info);
    if (err != ESP_OK) {
        return err;
    }
    if (decoded_info.width != expected_info.width ||
        decoded_info.height != expected_info.height ||
        decoded_info.output_len >
            preprocessor->config.max_decode_pixels * 3U) {
        return ESP_ERR_INVALID_SIZE;
    }

    convert_and_resize(preprocessor, &decoded_info, &source_info, image);
    return ESP_OK;
}

void image_preprocessor_cancel_staged(
    image_preprocessor_handle_t preprocessor)
{
    if (preprocessor != NULL) {
        preprocessor->staged = false;
    }
}
