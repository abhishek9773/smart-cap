#include "system_status.h"

#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static system_status_snapshot_t s_status;
static int64_t s_boot_time_us;
static int64_t s_last_frame_time_us;

void system_status_init(void)
{
    const int64_t boot_time_us = esp_timer_get_time();
    portENTER_CRITICAL(&s_lock);
    memset(&s_status, 0, sizeof(s_status));
    snprintf(s_status.mode, sizeof(s_status.mode), "%s", "vibration_sound");
    snprintf(s_status.mode_source, sizeof(s_status.mode_source), "%s", "default");
    s_boot_time_us = boot_time_us;
    s_last_frame_time_us = 0;
    portEXIT_CRITICAL(&s_lock);
}

void system_status_set_camera_initialized(bool initialized)
{
    portENTER_CRITICAL(&s_lock);
    s_status.camera_initialized = initialized;
    portEXIT_CRITICAL(&s_lock);
}

void system_status_record_capture(bool succeeded, uint32_t latency_ms)
{
    const int64_t capture_time_us = succeeded ? esp_timer_get_time() : 0;
    portENTER_CRITICAL(&s_lock);
    s_status.last_capture_ok = succeeded;
    if (succeeded) {
        ++s_status.frames_captured;
        s_status.capture_latency_ms = latency_ms;
        s_last_frame_time_us = capture_time_us;
    } else {
        ++s_status.frames_failed;
    }
    portEXIT_CRITICAL(&s_lock);
}

void system_status_set_mode(const char *mode, const char *source)
{
    if (mode == NULL || source == NULL) {
        return;
    }

    portENTER_CRITICAL(&s_lock);
    snprintf(s_status.mode, sizeof(s_status.mode), "%s", mode);
    snprintf(s_status.mode_source, sizeof(s_status.mode_source), "%s", source);
    portEXIT_CRITICAL(&s_lock);
}

void system_status_set_last_error(uint32_t error_code)
{
    portENTER_CRITICAL(&s_lock);
    s_status.last_error_code = error_code;
    portEXIT_CRITICAL(&s_lock);
}

void system_status_get_snapshot(system_status_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return;
    }

    const int64_t now_us = esp_timer_get_time();
    portENTER_CRITICAL(&s_lock);
    *snapshot = s_status;
    const int64_t boot_time_us = s_boot_time_us;
    const int64_t last_frame_time_us = s_last_frame_time_us;
    portEXIT_CRITICAL(&s_lock);

    snapshot->uptime_ms = (uint64_t)((now_us - boot_time_us) / 1000);
    snapshot->last_frame_age_ms = last_frame_time_us == 0
        ? 0
        : (uint32_t)((now_us - last_frame_time_us) / 1000);
    snapshot->free_heap_bytes = esp_get_free_heap_size();
    snapshot->minimum_free_heap_bytes = esp_get_minimum_free_heap_size();
    snapshot->largest_free_heap_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
}
