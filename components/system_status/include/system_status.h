#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define SYSTEM_STATUS_MODE_MAX 24
#define SYSTEM_STATUS_SOURCE_MAX 16

typedef struct {
    bool camera_initialized;
    bool last_capture_ok;
    uint32_t last_frame_age_ms;
    uint32_t capture_latency_ms;
    uint32_t frames_captured;
    uint32_t frames_failed;
    char mode[SYSTEM_STATUS_MODE_MAX];
    char mode_source[SYSTEM_STATUS_SOURCE_MAX];
    uint32_t last_error_code;
    uint32_t free_heap_bytes;
    uint32_t minimum_free_heap_bytes;
    uint32_t largest_free_heap_block;
    uint64_t uptime_ms;
} system_status_snapshot_t;

void system_status_init(void);
void system_status_set_camera_initialized(bool initialized);
void system_status_record_capture(bool succeeded, uint32_t latency_ms);
void system_status_set_mode(const char *mode, const char *source);
void system_status_set_last_error(uint32_t error_code);
void system_status_get_snapshot(system_status_snapshot_t *snapshot);
