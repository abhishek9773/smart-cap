#include "event_manager.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static char s_events[EVENT_MANAGER_CAPACITY][EVENT_MANAGER_NAME_MAX];
static size_t s_count;
static size_t s_next;

static const char *event_name(event_manager_event_t event)
{
    switch (event) {
    case EVENT_DEVICE_BOOT: return "device_boot";
    case EVENT_WIFI_AP_STARTED: return "wifi_ap_started";
    case EVENT_CAMERA_READY: return "camera_ready";
    case EVENT_CAMERA_FAILURE: return "camera_failure";
    case EVENT_AUDIO_REJECTED: return "audio_rejected";
    case EVENT_HAPTICS_REJECTED: return "haptics_rejected";
    default: return "unknown_event";
    }
}

void event_manager_init(void)
{
    portENTER_CRITICAL(&s_lock);
    memset(s_events, 0, sizeof(s_events));
    s_count = 0;
    s_next = 0;
    portEXIT_CRITICAL(&s_lock);
}

void event_manager_record(event_manager_event_t event)
{
    portENTER_CRITICAL(&s_lock);
    snprintf(s_events[s_next], sizeof(s_events[s_next]), "%s", event_name(event));
    s_next = (s_next + 1U) % EVENT_MANAGER_CAPACITY;
    if (s_count < EVENT_MANAGER_CAPACITY) {
        ++s_count;
    }
    portEXIT_CRITICAL(&s_lock);
}

size_t event_manager_copy(char events[][EVENT_MANAGER_NAME_MAX], size_t capacity)
{
    if (events == NULL || capacity == 0) {
        return 0;
    }

    portENTER_CRITICAL(&s_lock);
    const size_t count = s_count < capacity ? s_count : capacity;
    const size_t first = (s_next + EVENT_MANAGER_CAPACITY - s_count) % EVENT_MANAGER_CAPACITY;
    for (size_t i = 0; i < count; ++i) {
        const size_t index = (first + i) % EVENT_MANAGER_CAPACITY;
        snprintf(events[i], EVENT_MANAGER_NAME_MAX, "%s", s_events[index]);
    }
    portEXIT_CRITICAL(&s_lock);
    return count;
}
