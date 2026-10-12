#include "camera_test.h"

#include <stdint.h>

#include "board_config.h"
#include "esp_camera.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"

#define CAMERA_TEST_DURATION_US (30LL * 1000000LL)

static const char *TAG = "CAMERA_TEST";

static camera_config_t make_camera_config(void)
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
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 12,
        .fb_count = 2,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST,
    };

    return config;
}

bool camera_test_run(
    i2c_master_dev_handle_t oled_device,
    bool oled_ready)
{
    const camera_config_t config = make_camera_config();
    const esp_err_t init_err = esp_camera_init(&config);

    if (init_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Camera initialization failed: %s (0x%x)",
            esp_err_to_name(init_err),
            (unsigned int)init_err);
        return false;
    }

    ESP_LOGI(
        TAG,
        "Camera initialized: QVGA 320x240, JPEG, XCLK=%uHz, "
        "frame_buffers=%u in PSRAM, grab_mode=LATEST",
        (unsigned int)config.xclk_freq_hz,
        (unsigned int)config.fb_count);

    uint32_t window_frames = 0;
    uint32_t window_failures = 0;
    uint32_t total_frames = 0;
    uint32_t total_failures = 0;
    uint64_t window_jpeg_bytes = 0;
    uint64_t total_jpeg_bytes = 0;
    const int64_t test_start_us = esp_timer_get_time();
    int64_t window_start_us = esp_timer_get_time();
    int64_t last_oled_update_us = window_start_us;
    uint32_t report_count = 0;
    bool first_frame_logged = false;
    static const uint8_t patterns[] = {0xAA, 0x55, 0xFF, 0x00};

    while (esp_timer_get_time() - test_start_us < CAMERA_TEST_DURATION_US) {
        camera_fb_t *frame = esp_camera_fb_get();

        if (frame == NULL) {
            ++window_failures;
            ++total_failures;
            ESP_LOGW(TAG, "Frame capture returned no frame");
        } else {
            if (!first_frame_logged) {
                const char *format_name =
                    frame->format == PIXFORMAT_JPEG ? "JPEG" : "non-JPEG";
                ESP_LOGI(
                    TAG,
                    "First frame: width=%u, height=%u, pixel_format=%d "
                    "(%s), length=%u bytes",
                    (unsigned int)frame->width,
                    (unsigned int)frame->height,
                    (int)frame->format,
                    format_name,
                    (unsigned int)frame->len);
                first_frame_logged = true;
            }

            ++window_frames;
            ++total_frames;
            window_jpeg_bytes += frame->len;
            total_jpeg_bytes += frame->len;
            esp_camera_fb_return(frame);
        }

        const int64_t now_us = esp_timer_get_time();

        if (now_us - window_start_us >= CAMERA_FPS_REPORT_INTERVAL_US) {
            const uint64_t elapsed_us =
                (uint64_t)(now_us - window_start_us);
            const uint32_t window_fps_x100 = (uint32_t)(
                ((uint64_t)window_frames * 100000000ULL) / elapsed_us);
            const uint32_t average_jpeg_bytes =
                window_frames == 0
                    ? 0
                    : (uint32_t)(window_jpeg_bytes / window_frames);

            ESP_LOGI(
                TAG,
                "CAMERA window: %lu.%02lu FPS | %lu frames | "
                "%lu failed | avg JPEG %lu bytes",
                (unsigned long)(window_fps_x100 / 100U),
                (unsigned long)(window_fps_x100 % 100U),
                (unsigned long)window_frames,
                (unsigned long)window_failures,
                (unsigned long)average_jpeg_bytes);

            window_frames = 0;
            window_failures = 0;
            window_jpeg_bytes = 0;
            window_start_us = now_us;
            ++report_count;
        }

        if (oled_ready &&
            now_us - last_oled_update_us >= OLED_PATTERN_INTERVAL_US) {
            const uint8_t pattern =
                patterns[report_count % (sizeof(patterns) / sizeof(patterns[0]))];
            const esp_err_t display_err =
                ssd1306_show_test_pattern(oled_device, pattern);

            if (display_err != ESP_OK) {
                ESP_LOGE(
                    TAG,
                    "OLED update failed: %s",
                    esp_err_to_name(display_err));
                oled_ready = false;
            } else {
                ESP_LOGI(TAG, "OLED test pattern: 0x%02x", pattern);
            }

            last_oled_update_us = now_us;
        }

        if (frame == NULL &&
            esp_timer_get_time() - test_start_us < CAMERA_TEST_DURATION_US) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    const int64_t test_elapsed_us = esp_timer_get_time() - test_start_us;
    const uint32_t total_fps_x100 = test_elapsed_us > 0
        ? (uint32_t)(
            ((uint64_t)total_frames * 100000000ULL) /
            (uint64_t)test_elapsed_us)
        : 0;
    const uint32_t average_total_jpeg_bytes = total_frames == 0
        ? 0
        : (uint32_t)(total_jpeg_bytes / total_frames);

    ESP_LOGI(
        TAG,
        "CAMERA test complete: duration=%lldms | %lu.%02lu FPS | "
        "%lu successful | %lu failed | avg JPEG %lu bytes",
        (long long)(test_elapsed_us / 1000),
        (unsigned long)(total_fps_x100 / 100U),
        (unsigned long)(total_fps_x100 % 100U),
        (unsigned long)total_frames,
        (unsigned long)total_failures,
        (unsigned long)average_total_jpeg_bytes);

    const esp_err_t deinit_err = esp_camera_deinit();
    if (deinit_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Camera deinitialization failed: %s",
            esp_err_to_name(deinit_err));
        return false;
    }

    return total_frames > 0;
}
