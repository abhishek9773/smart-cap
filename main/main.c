#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "axp2101.h"
#include "board_config.h"
#include "camera_pipeline.h"
#include "i2c_bus.h"
#include "ssd1306.h"

#define PREPROCESSING_TEST_DURATION_US (30LL * 1000000LL)

static const char *TAG = "SMART_CAP_HW_TEST";

static bool run_preprocessing_test(
    i2c_master_dev_handle_t oled_device,
    bool oled_ready)
{
    const camera_pipeline_config_t config = {
        .camera = {
            .frame_size = FRAMESIZE_QVGA,
            .jpeg_quality = 12,
            .framebuffer_count = 2,
        },
        .image = {
            .max_jpeg_bytes = 128U * 1024U,
            .max_decode_pixels = 320U * 240U,
            .max_output_pixels = 160U * 120U,
            .output_width = 160,
            .output_height = 120,
            .output_format = IMAGE_PIXEL_FORMAT_RGB565,
        },
    };
    camera_pipeline_handle_t pipeline = NULL;
    esp_err_t err = camera_pipeline_create(&config, &pipeline);
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Camera preprocessing pipeline initialization failed: %s",
            esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(
        TAG,
        "Preprocessing test started: QVGA JPEG -> 160x120 RGB565; "
        "30-second demand-driven run, latest-frame capture, no timer");

    uint32_t processed_frames = 0;
    uint32_t failed_frames = 0;
    uint64_t total_output_bytes = 0;
    bool first_image_logged = false;
    const int64_t start_us = esp_timer_get_time();
    int64_t last_oled_update_us = start_us;
    uint32_t oled_pattern_index = 0;
    static const uint8_t oled_patterns[] = {0xAA, 0x55, 0xFF, 0x00};

    while (esp_timer_get_time() - start_us < PREPROCESSING_TEST_DURATION_US) {
        image_preprocessed_image_t image = {0};
        err = camera_pipeline_process_next(pipeline, &image);
        if (err != ESP_OK) {
            ++failed_frames;
            ESP_LOGW(
                TAG,
                "Frame preprocessing failed: %s",
                esp_err_to_name(err));
            continue;
        }

        ++processed_frames;
        total_output_bytes += image.len;
        if (!first_image_logged) {
            ESP_LOGI(
                TAG,
                "First preprocessed image: input=%ux%u, output=%ux%u, "
                "format=RGB565, length=%u bytes",
                (unsigned int)image.source_width,
                (unsigned int)image.source_height,
                (unsigned int)image.width,
                (unsigned int)image.height,
                (unsigned int)image.len);
            first_image_logged = true;
        }

        const int64_t now_us = esp_timer_get_time();
        if (oled_ready &&
            now_us - last_oled_update_us >= OLED_PATTERN_INTERVAL_US) {
            const uint8_t pattern = oled_patterns[
                oled_pattern_index %
                (sizeof(oled_patterns) / sizeof(oled_patterns[0]))];
            const esp_err_t display_err =
                ssd1306_show_test_pattern(oled_device, pattern);
            if (display_err != ESP_OK) {
                ESP_LOGE(
                    TAG,
                    "OLED update failed: %s",
                    esp_err_to_name(display_err));
                oled_ready = false;
            } else {
                ++oled_pattern_index;
            }
            last_oled_update_us = now_us;
        }
    }

    const int64_t elapsed_us = esp_timer_get_time() - start_us;
    const uint32_t average_output_bytes = processed_frames == 0
        ? 0
        : (uint32_t)(total_output_bytes / processed_frames);
    ESP_LOGI(
        TAG,
        "Preprocessing test complete: duration=%lldms | %lu processed | "
        "%lu failed | average output=%lu bytes",
        (long long)(elapsed_us / 1000),
        (unsigned long)processed_frames,
        (unsigned long)failed_frames,
        (unsigned long)average_output_bytes);

    const esp_err_t destroy_err = camera_pipeline_destroy(pipeline);
    if (destroy_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Camera preprocessing pipeline cleanup failed: %s",
            esp_err_to_name(destroy_err));
        return false;
    }
    return processed_frames > 0;
}

static void initialize_optional_oled(
    i2c_master_dev_handle_t *oled_device,
    bool *oled_ready)
{
    esp_err_t err = i2c_bus_add_device(
        SSD1306_I2C_ADDRESS,
        oled_device);

    if (err == ESP_OK) {
        err = ssd1306_initialize(*oled_device);
    }

    if (err == ESP_OK) {
        err = ssd1306_show_test_pattern(*oled_device, 0xAA);
    }

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "OLED initialization/test failed: %s",
            esp_err_to_name(err));

        const esp_err_t remove_err = i2c_bus_remove_device(oled_device);
        if (remove_err != ESP_OK) {
            ESP_LOGE(
                TAG,
                "OLED cleanup failed: %s",
                esp_err_to_name(remove_err));
        }
        return;
    }

    *oled_ready = true;
    ESP_LOGI(
        TAG,
        "OLED responded at address 0x%02x",
        SSD1306_I2C_ADDRESS);
}

static void check_optional_pmu(void)
{
    i2c_master_dev_handle_t pmu_device = NULL;
    esp_err_t err = i2c_bus_add_device(
        AXP2101_I2C_ADDRESS,
        &pmu_device);

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "PMU registration failed: %s",
            esp_err_to_name(err));
        return;
    }

    uint8_t chip_id = 0;
    err = axp2101_read_chip_id(pmu_device, &chip_id);

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "PMU chip-ID read failed: %s",
            esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "PMU chip ID register: 0x%02x", chip_id);

        if (chip_id != AXP2101_CHIP_ID) {
            ESP_LOGW(
                TAG,
                "PMU ID differs from expected AXP2101 ID. "
                "Camera power rails will not be configured.");
        } else {
            err = axp2101_configure_camera_rails(pmu_device);
            if (err != ESP_OK) {
                ESP_LOGE(
                    TAG,
                    "Camera power-rail configuration failed: %s",
                    esp_err_to_name(err));
            } else {
                ESP_LOGI(
                    TAG,
                    "Camera rails enabled using LilyGO factory settings: "
                    "ALDO1=1500mV, ALDO2=3000mV, ALDO4=3000mV");
            }
        }
    }

    err = i2c_bus_remove_device(&pmu_device);
    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "PMU device cleanup failed: %s",
            esp_err_to_name(err));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "LILYGO T-Camera S3 V1.6 hardware test");

    const esp_err_t i2c_err = i2c_bus_initialize();
    if (i2c_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "I2C initialization failed: %s",
            esp_err_to_name(i2c_err));

        if (!run_preprocessing_test(NULL, false)) {
            ESP_LOGE(TAG, "Camera preprocessing test failed");
        }
        return;
    }

    i2c_master_dev_handle_t oled_device = NULL;
    bool oled_ready = false;
    initialize_optional_oled(&oled_device, &oled_ready);
    check_optional_pmu();

    ESP_LOGI(
        TAG,
        "Starting camera pipeline; verified camera rails were configured if "
        "AXP2101 was identified.");

    const bool preprocessing_test_ok =
        run_preprocessing_test(oled_device, oled_ready);
    if (!preprocessing_test_ok) {
        ESP_LOGE(TAG, "Camera preprocessing test failed");
    }

    esp_err_t cleanup_err = i2c_bus_remove_device(&oled_device);
    if (cleanup_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "OLED cleanup failed: %s",
            esp_err_to_name(cleanup_err));
    }

    cleanup_err = i2c_bus_deinitialize();
    if (cleanup_err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "I2C bus cleanup failed: %s",
            esp_err_to_name(cleanup_err));
    }
}
