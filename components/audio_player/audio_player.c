#include "audio_player.h"

#include <stdint.h>
#include <string.h>

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
        ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[3] << 24);
}

esp_err_t audio_player_init(void)
{
    return ESP_OK;
}

bool audio_player_is_configured(void)
{
    return false;
}

audio_player_wav_result_t audio_player_validate_wav(const uint8_t *data, size_t length)
{
    if (data == NULL || length < 12 || length > AUDIO_PLAYER_MAX_UPLOAD_BYTES) {
        return AUDIO_WAV_MALFORMED;
    }
    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) {
        return AUDIO_WAV_UNSUPPORTED;
    }

    const uint32_t riff_length = read_le32(data + 4);
    if ((uint64_t)riff_length + 8U != length) {
        return AUDIO_WAV_MALFORMED;
    }

    bool has_supported_format = false;
    bool has_data = false;
    size_t offset = 12;

    while (offset < length) {
        if (length - offset < 8) {
            return AUDIO_WAV_MALFORMED;
        }

        const uint8_t *chunk_id = data + offset;
        const uint32_t chunk_length = read_le32(data + offset + 4);
        const size_t chunk_data = offset + 8;
        if ((uint64_t)chunk_data + chunk_length > length) {
            return AUDIO_WAV_MALFORMED;
        }

        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            if (chunk_length < 16) {
                return AUDIO_WAV_MALFORMED;
            }
            const uint8_t *fmt = data + chunk_data;
            has_supported_format =
                read_le16(fmt) == 1 &&
                read_le16(fmt + 2) == 1 &&
                read_le32(fmt + 4) == 16000 &&
                read_le32(fmt + 8) == 32000 &&
                read_le16(fmt + 12) == 2 &&
                read_le16(fmt + 14) == 16;
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            has_data = chunk_length > 0 && (chunk_length % 2U) == 0;
        }

        const size_t padded_chunk_length = (size_t)chunk_length + (chunk_length & 1U);
        if (padded_chunk_length > length - chunk_data) {
            return AUDIO_WAV_MALFORMED;
        }
        offset = chunk_data + padded_chunk_length;
    }

    if (offset != length) {
        return AUDIO_WAV_MALFORMED;
    }
    if (!has_supported_format) {
        return AUDIO_WAV_UNSUPPORTED;
    }
    return has_data ? AUDIO_WAV_VALID : AUDIO_WAV_MALFORMED;
}

void audio_player_get_status(audio_player_status_t *status)
{
    if (status != NULL) {
        status->configured = false;
        status->ready = false;
        status->playing = false;
        status->queued_jobs = 0;
        status->last_playback_id = 0;
        status->last_result = "not_configured";
    }
}

esp_err_t audio_player_stop(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
