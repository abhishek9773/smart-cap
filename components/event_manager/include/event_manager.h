#pragma once

#include <stddef.h>

typedef enum {
    EVENT_DEVICE_BOOT,
    EVENT_WIFI_AP_STARTED,
    EVENT_CAMERA_READY,
    EVENT_CAMERA_FAILURE,
    EVENT_AUDIO_REJECTED,
    EVENT_HAPTICS_REJECTED,
} event_manager_event_t;

#define EVENT_MANAGER_CAPACITY 16
#define EVENT_MANAGER_NAME_MAX 32

void event_manager_init(void);
void event_manager_record(event_manager_event_t event);
size_t event_manager_copy(char events[][EVENT_MANAGER_NAME_MAX], size_t capacity);
