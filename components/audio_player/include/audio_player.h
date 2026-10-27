#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define AUDIO_PLAYER_MAX_UPLOAD_BYTES (256U * 1024U)

typedef enum {
    AUDIO_WAV_VALID,
    AUDIO_WAV_MALFORMED,
    AUDIO_WAV_UNSUPPORTED,
} audio_player_wav_result_t;

typedef struct {
    bool configured;
    bool ready;
    bool playing;
    size_t queued_jobs;
    uint32_t last_playback_id;
    const char *last_result;
} audio_player_status_t;

esp_err_t audio_player_init(void);
bool audio_player_is_configured(void);
audio_player_wav_result_t audio_player_validate_wav(const uint8_t *data, size_t length);
void audio_player_get_status(audio_player_status_t *status);
esp_err_t audio_player_stop(void);
