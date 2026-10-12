#include "camera_driver.h"

#include <stdbool.h>

#include "board_config.h"

static bool s_initialized;

static camera_config_t make_camera_config(
    const camera_driver_config_t *driver_config)
{
    const camera_config_t config = {
        .pin_pwdn = -1,
        .pin_reset = BOARD_CAMERA_PIN_RESET,
        .pin_xclk = BOARD_CAMERA_PIN_XCLK,
        .pin_sccb_sda = BOARD_CAMERA_PIN_SCCB_SDA,
        .pin_sccb_scl = BOARD_CAMERA_PIN_SCCB_SCL,
        .pin_d0 = BOARD_CAMERA_PIN_D0,
        .pin_d1 = BOARD_CAMERA_PIN_D1,
        .pin_d2 = BOARD_CAMERA_PIN_D2,
        .pin_d3 = BOARD_CAMERA_PIN_D3,
        .pin_d4 = BOARD_CAMERA_PIN_D4,
        .pin_d5 = BOARD_CAMERA_PIN_D5,
        .pin_d6 = BOARD_CAMERA_PIN_D6,
        .pin_d7 = BOARD_CAMERA_PIN_D7,
        .pin_vsync = BOARD_CAMERA_PIN_VSYNC,
        .pin_href = BOARD_CAMERA_PIN_HREF,
        .pin_pclk = BOARD_CAMERA_PIN_PCLK,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = driver_config->frame_size,
        .jpeg_quality = driver_config->jpeg_quality,
        .fb_count = driver_config->framebuffer_count,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST,
    };

    return config;
}

esp_err_t camera_driver_init(const camera_driver_config_t *driver_config)
{
    if (driver_config == NULL ||
        driver_config->jpeg_quality > 63 ||
        driver_config->framebuffer_count == 0 ||
        driver_config->framebuffer_count > 2) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    const camera_config_t config = make_camera_config(driver_config);
    const esp_err_t err = esp_camera_init(&config);
    if (err == ESP_OK) {
        s_initialized = true;
    }
    return err;
}

esp_err_t camera_driver_acquire(camera_fb_t **frame)
{
    if (frame == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *frame = NULL;

    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    *frame = esp_camera_fb_get();
    return *frame == NULL ? ESP_FAIL : ESP_OK;
}

esp_err_t camera_driver_release(camera_fb_t *frame)
{
    if (frame == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_camera_fb_return(frame);
    return ESP_OK;
}

esp_err_t camera_driver_deinit(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    const esp_err_t err = esp_camera_deinit();
    if (err == ESP_OK) {
        s_initialized = false;
    }
    return err;
}
