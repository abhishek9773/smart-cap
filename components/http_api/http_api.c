    #include "http_api.h"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <strings.h>
    #include <ctype.h>

    #include "audio_player.h"
    #include "button_manager.h"
    #include "camera_driver.h"
    #include "cJSON.h"
    #include "event_manager.h"
    #include "esp_heap_caps.h"
    #include "esp_http_server.h"
    #include "esp_log.h"
    #include "esp_timer.h"
    #include "haptics.h"
    #include "sensor_manager.h"
    #include "system_status.h"
    #include "wifi_manager.h"

    #define HTTP_API_MAX_JSON_BODY 512U
    #define HTTP_API_MAX_FRAME_RATE_INTERVAL_US 500000LL
    #define HTTP_API_MAX_JPEG_BYTES (256U * 1024U)
    #define HTTP_API_JSON_BUFFER 1280U

    static const char *TAG = "http_api";
    static httpd_handle_t s_server;
    static int64_t s_last_frame_request_us;

    static void set_http_status(httpd_req_t *req, int status_code)
    {
        switch (status_code) {
        case 202: httpd_resp_set_status(req, "202 Accepted"); break;
        case 400: httpd_resp_set_status(req, "400 Bad Request"); break;
        case 404: httpd_resp_set_status(req, "404 Not Found"); break;
        case 405: httpd_resp_set_status(req, "405 Method Not Allowed"); break;
        case 409: httpd_resp_set_status(req, "409 Conflict"); break;
        case 413: httpd_resp_set_status(req, "413 Payload Too Large"); break;
        case 415: httpd_resp_set_status(req, "415 Unsupported Media Type"); break;
        case 429: httpd_resp_set_status(req, "429 Too Many Requests"); break;
        case 500: httpd_resp_set_status(req, "500 Internal Server Error"); break;
        case 503: httpd_resp_set_status(req, "503 Service Unavailable"); break;
        default: httpd_resp_set_status(req, "200 OK"); break;
        }
    }

    static esp_err_t send_json(httpd_req_t *req, const char *json)
    {
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    }

    static esp_err_t send_error(
        httpd_req_t *req,
        int status_code,
        const char *code,
        const char *message)
    {
        char json[384];
        set_http_status(req, status_code);
        snprintf(
            json,
            sizeof(json),
            "{\"ok\":false,\"data\":null,\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}",
            code,
            message);
        return send_json(req, json);
    }

    static esp_err_t send_data(httpd_req_t *req, const char *data_json)
    {
        char json[HTTP_API_JSON_BUFFER];
        const int written = snprintf(
            json,
            sizeof(json),
            "{\"ok\":true,\"data\":%s,\"error\":null}",
            data_json);
        if (written < 0 || (size_t)written >= sizeof(json)) {
            return send_error(req, 500, "INTERNAL_ERROR", "Response could not be encoded");
        }
        return send_json(req, json);
    }

    static bool escape_json_string(const char *input, char *output, size_t output_size)
    {
        size_t written = 0;
        for (const unsigned char *cursor = (const unsigned char *)input; *cursor != '\0'; ++cursor) {
            const char *escaped = NULL;
            switch (*cursor) {
            case '"': escaped = "\\\""; break;
            case '\\': escaped = "\\\\"; break;
            case '\b': escaped = "\\b"; break;
            case '\f': escaped = "\\f"; break;
            case '\n': escaped = "\\n"; break;
            case '\r': escaped = "\\r"; break;
            case '\t': escaped = "\\t"; break;
            default: break;
            }

            if (escaped != NULL) {
                const size_t length = strlen(escaped);
                if (length >= output_size - written) {
                    return false;
                }
                memcpy(output + written, escaped, length);
                written += length;
            } else if (*cursor < 0x20) {
                if (output_size - written <= 6) {
                    return false;
                }
                const int length = snprintf(output + written, output_size - written, "\\u%04x", *cursor);
                if (length != 6) {
                    return false;
                }
                written += (size_t)length;
            } else {
                if (output_size - written <= 1) {
                    return false;
                }
                output[written++] = (char)*cursor;
            }
        }

        if (written >= output_size) {
            return false;
        }
        output[written] = '\0';
        return true;
    }

    static esp_err_t receive_body(httpd_req_t *req, size_t max_length, uint8_t **body, size_t *length)
    {
        if (body == NULL || length == NULL) {
            return ESP_ERR_INVALID_ARG;
        }
        *body = NULL;
        *length = 0;

        if (req->content_len == 0) {
            return ESP_ERR_INVALID_SIZE;
        }
        if (req->content_len > max_length) {
            return ESP_ERR_INVALID_SIZE;
        }

        uint8_t *buffer = malloc(req->content_len + 1U);
        if (buffer == NULL) {
            return ESP_ERR_NO_MEM;
        }

        size_t received = 0;
        while (received < req->content_len) {
            const int result = httpd_req_recv(
                req,
                (char *)buffer + received,
                req->content_len - received);
            if (result <= 0) {
                free(buffer);
                return ESP_FAIL;
            }
            received += (size_t)result;
        }

        buffer[received] = '\0';
        *body = buffer;
        *length = received;
        return ESP_OK;
    }

    static cJSON *parse_json_body(httpd_req_t *req, size_t max_length, int *error_status)
    {
        uint8_t *body = NULL;
        size_t length = 0;
        const esp_err_t err = receive_body(req, max_length, &body, &length);
        if (err != ESP_OK) {
            *error_status = err == ESP_ERR_INVALID_SIZE && req->content_len > max_length ? 413 : 400;
            return NULL;
        }

        const char *parse_end = NULL;
        cJSON *json = cJSON_ParseWithLengthOpts(
            (const char *)body,
            length,
            &parse_end,
            false);
        if (json != NULL && parse_end != NULL) {
            while (parse_end < (const char *)body + length && isspace((unsigned char)*parse_end)) {
                ++parse_end;
            }
            if (parse_end != (const char *)body + length) {
                cJSON_Delete(json);
                json = NULL;
            }
        }
        free(body);
        if (json == NULL) {
            *error_status = 400;
        }
        return json;
    }

    static esp_err_t health_handler(httpd_req_t *req)
    {
        system_status_snapshot_t state;
        system_status_get_snapshot(&state);

        char data[256];
        snprintf(
            data,
            sizeof(data),
            "{\"service\":\"smart-cap\",\"status\":\"healthy\",\"firmware_version\":\"1.0.0\",\"uptime_ms\":%llu,\"api_version\":\"v1\"}",
            (unsigned long long)state.uptime_ms);
        return send_data(req, data);
    }

    static esp_err_t status_handler(httpd_req_t *req)
    {
        system_status_snapshot_t state;
        wifi_manager_status_t wifi;
        audio_player_status_t audio;
        haptics_status_t haptics;
        sensor_manager_status_t sensors;
        system_status_get_snapshot(&state);
        wifi_manager_get_status(&wifi);
        audio_player_get_status(&audio);
        haptics_get_status(&haptics);
        sensor_manager_get_status(&sensors);

        char data[HTTP_API_JSON_BUFFER];
        char escaped_ssid[65];
        char escaped_ip[32];
        if (!escape_json_string(wifi.ssid, escaped_ssid, sizeof(escaped_ssid)) ||
            !escape_json_string(wifi.ip_address, escaped_ip, sizeof(escaped_ip))) {
            return send_error(req, 500, "INTERNAL_ERROR", "Network status could not be encoded");
        }
        char last_error[16];
        char battery_voltage[16];
        char battery_percentage[16];
        const char *charging = "null";
        if (state.last_error_code == 0) {
            snprintf(last_error, sizeof(last_error), "%s", "null");
        } else {
            snprintf(last_error, sizeof(last_error), "%lu", (unsigned long)state.last_error_code);
        }
        if (sensors.battery_measurement_available) {
            snprintf(battery_voltage, sizeof(battery_voltage), "%d", sensors.battery_voltage_mv);
        } else {
            snprintf(battery_voltage, sizeof(battery_voltage), "%s", "null");
        }
        if (sensors.battery_percentage_valid &&
            sensors.battery_percentage >= 0 &&
            sensors.battery_percentage <= 100) {
            snprintf(battery_percentage, sizeof(battery_percentage), "%d", sensors.battery_percentage);
        } else {
            snprintf(battery_percentage, sizeof(battery_percentage), "%s", "null");
        }
        if (sensors.charging_status_available) {
            charging = sensors.charging ? "true" : "false";
        }
        const int written = snprintf(
            data,
            sizeof(data),
            "{\"device\":\"smart-cap\",\"firmware_version\":\"1.0.0\",\"api_version\":\"v1\",\"uptime_ms\":%llu,"
            "\"wifi\":{\"ap_active\":%s,\"ssid\":\"%s\",\"ip\":\"%s\",\"connected_clients\":%d},"
            "\"camera\":{\"initialized\":%s,\"last_capture_ok\":%s,\"last_frame_age_ms\":%lu,\"capture_latency_ms\":%lu,\"frames_captured\":%lu,\"frames_failed\":%lu},"
            "\"audio\":{\"configured\":%s,\"ready\":%s,\"playing\":%s},"
            "\"haptics\":{\"configured\":%s,\"left_active\":%s,\"right_active\":%s},"
            "\"mode\":{\"value\":\"%s\",\"source\":\"%s\",\"physical_switch_available\":%s},"
            "\"battery\":{\"measurement_available\":%s,\"voltage_mv\":%s,\"percentage\":%s,\"charging\":%s},"
            "\"memory\":{\"free_heap_bytes\":%lu,\"minimum_free_heap_bytes\":%lu,\"largest_free_heap_block\":%lu},"
            "\"last_error_code\":%s}",
            (unsigned long long)state.uptime_ms,
            wifi.active ? "true" : "false",
            escaped_ssid,
            escaped_ip,
            wifi.connected_clients,
            state.camera_initialized ? "true" : "false",
            state.last_capture_ok ? "true" : "false",
            (unsigned long)state.last_frame_age_ms,
            (unsigned long)state.capture_latency_ms,
            (unsigned long)state.frames_captured,
            (unsigned long)state.frames_failed,
            audio.configured ? "true" : "false",
            audio.ready ? "true" : "false",
            audio.playing ? "true" : "false",
            haptics.configured ? "true" : "false",
            haptics.left_active ? "true" : "false",
            haptics.right_active ? "true" : "false",
            state.mode,
            state.mode_source,
            button_manager_mode_switch_available() ? "true" : "false",
            sensors.battery_measurement_available ? "true" : "false",
            battery_voltage,
            battery_percentage,
            charging,
            (unsigned long)state.free_heap_bytes,
            (unsigned long)state.minimum_free_heap_bytes,
            (unsigned long)state.largest_free_heap_block,
            last_error);
        if (written < 0 || (size_t)written >= sizeof(data)) {
            return send_error(req, 500, "INTERNAL_ERROR", "Status response could not be encoded");
        }
        return send_data(req, data);
    }

    static esp_err_t frame_handler(httpd_req_t *req)
    {
        const int64_t now_us = esp_timer_get_time();
        if (s_last_frame_request_us != 0 &&
            now_us - s_last_frame_request_us < HTTP_API_MAX_FRAME_RATE_INTERVAL_US) {
            return send_error(req, 429, "FRAME_RATE_LIMIT", "Request frames no faster than 2 FPS");
        }
        s_last_frame_request_us = now_us;

        system_status_snapshot_t state;
        system_status_get_snapshot(&state);
        if (!state.camera_initialized) {
            return send_error(req, 503, "CAMERA_NOT_READY", "Camera is not initialized");
        }

        const int64_t capture_start_us = esp_timer_get_time();
        camera_fb_t *frame = NULL;
        esp_err_t err = camera_driver_acquire(&frame);
        if (err != ESP_OK || frame == NULL) {
            system_status_record_capture(false, 0);
            return send_error(req, 503, "CAMERA_CAPTURE_FAILED", "Camera could not capture a frame");
        }

        if (frame->buf == NULL || frame->len == 0 || frame->format != PIXFORMAT_JPEG) {
            camera_driver_release(frame);
            system_status_record_capture(false, 0);
            return send_error(req, 500, "CAMERA_INVALID_FRAME", "Camera returned an invalid JPEG frame");
        }
        if (frame->len > HTTP_API_MAX_JPEG_BYTES) {
            camera_driver_release(frame);
            system_status_record_capture(false, 0);
            return send_error(req, 500, "CAMERA_FRAME_TOO_LARGE", "Camera JPEG exceeded the configured frame limit");
        }

        uint8_t *jpeg = heap_caps_malloc(frame->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (jpeg == NULL) {
            camera_driver_release(frame);
            system_status_record_capture(false, 0);
            return send_error(req, 503, "CAMERA_BUFFER_UNAVAILABLE", "Unable to allocate the bounded frame buffer");
        }
        const size_t jpeg_length = frame->len;
        memcpy(jpeg, frame->buf, jpeg_length);
        const esp_err_t release_err = camera_driver_release(frame);
        if (release_err != ESP_OK) {
            heap_caps_free(jpeg);
            system_status_record_capture(false, 0);
            return send_error(req, 500, "CAMERA_RELEASE_FAILED", "Camera frame buffer could not be released");
        }
        const uint32_t capture_latency_ms =
            (uint32_t)((esp_timer_get_time() - capture_start_us) / 1000);

        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        httpd_resp_set_hdr(req, "Pragma", "no-cache");
        httpd_resp_set_hdr(req, "Expires", "0");
        system_status_record_capture(true, capture_latency_ms);
        err = httpd_resp_send(req, (const char *)jpeg, jpeg_length);
        heap_caps_free(jpeg);
        return err;
    }

    static esp_err_t sensors_handler(httpd_req_t *req)
    {
        sensor_manager_status_t sensors;
        sensor_manager_get_status(&sensors);

        char data[384];
        char left_mm[16];
        char right_mm[16];
        char battery_voltage[16];
        char battery_percentage[16];
        const char *charging = "null";
        if (sensors.left_valid) {
            snprintf(left_mm, sizeof(left_mm), "%d", sensors.left_mm);
        } else {
            snprintf(left_mm, sizeof(left_mm), "%s", "null");
        }
        if (sensors.right_valid) {
            snprintf(right_mm, sizeof(right_mm), "%d", sensors.right_mm);
        } else {
            snprintf(right_mm, sizeof(right_mm), "%s", "null");
        }
        if (sensors.battery_measurement_available) {
            snprintf(battery_voltage, sizeof(battery_voltage), "%d", sensors.battery_voltage_mv);
        } else {
            snprintf(battery_voltage, sizeof(battery_voltage), "%s", "null");
        }
        if (sensors.battery_percentage_valid &&
            sensors.battery_percentage >= 0 &&
            sensors.battery_percentage <= 100) {
            snprintf(battery_percentage, sizeof(battery_percentage), "%d", sensors.battery_percentage);
        } else {
            snprintf(battery_percentage, sizeof(battery_percentage), "%s", "null");
        }
        if (sensors.charging_status_available) {
            charging = sensors.charging ? "true" : "false";
        }
        snprintf(
            data,
            sizeof(data),
            "{\"distance_sensors\":{\"available\":%s,\"left_mm\":%s,\"right_mm\":%s},"
            "\"battery\":{\"measurement_available\":%s,\"voltage_mv\":%s,\"percentage\":%s,\"charging\":%s}}",
            sensors.distance_sensors_available ? "true" : "false",
            left_mm,
            right_mm,
            sensors.battery_measurement_available ? "true" : "false",
            battery_voltage,
            battery_percentage,
            charging);
        return send_data(req, data);
    }

    static bool request_content_type_is_wav(httpd_req_t *req)
    {
        char content_type[64] = {0};
        if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type)) != ESP_OK) {
            return false;
        }
        return strcasecmp(content_type, "audio/wav") == 0 ||
            strcasecmp(content_type, "audio/wave") == 0 ||
            strncasecmp(content_type, "audio/wav;", 10) == 0;
    }

    static esp_err_t audio_handler(httpd_req_t *req)
    {
        if (req->content_len > AUDIO_PLAYER_MAX_UPLOAD_BYTES) {
            return send_error(req, 413, "PAYLOAD_TOO_LARGE", "Audio upload exceeds the configured limit");
        }
        if (!request_content_type_is_wav(req)) {
            return send_error(req, 415, "AUDIO_FORMAT_UNSUPPORTED", "Content-Type must be audio/wav");
        }

        uint8_t *body = NULL;
        size_t length = 0;
        const esp_err_t read_err = receive_body(req, AUDIO_PLAYER_MAX_UPLOAD_BYTES, &body, &length);
        if (read_err != ESP_OK) {
            const int code = read_err == ESP_ERR_INVALID_SIZE && req->content_len > AUDIO_PLAYER_MAX_UPLOAD_BYTES
                ? 413
                : read_err == ESP_ERR_NO_MEM ? 503 : 400;
            return send_error(req, code, code == 413 ? "PAYLOAD_TOO_LARGE" : "AUDIO_INVALID_WAV", "Audio request could not be read");
        }

        const audio_player_wav_result_t wav_result = audio_player_validate_wav(body, length);
        free(body);
        if (wav_result == AUDIO_WAV_MALFORMED) {
            return send_error(req, 400, "AUDIO_INVALID_WAV", "WAV chunks or lengths are malformed");
        }
        if (wav_result == AUDIO_WAV_UNSUPPORTED) {
            return send_error(req, 415, "AUDIO_FORMAT_UNSUPPORTED", "Expected mono 16-bit PCM WAV at 16 kHz");
        }
        if (!audio_player_is_configured()) {
            event_manager_record(EVENT_AUDIO_REJECTED);
            return send_error(req, 503, "AUDIO_NOT_CONFIGURED", "Audio output pins and I2S amplifier are not configured");
        }
        return send_error(req, 503, "AUDIO_NOT_READY", "Audio playback is not ready");
    }

    static esp_err_t audio_status_handler(httpd_req_t *req)
    {
        audio_player_status_t status;
        audio_player_get_status(&status);
        char data[256];
        snprintf(
            data,
            sizeof(data),
            "{\"configured\":%s,\"playing\":%s,\"queued_jobs\":%u,\"last_playback_id\":%u,\"last_result\":\"%s\"}",
            status.configured ? "true" : "false",
            status.playing ? "true" : "false",
            (unsigned)status.queued_jobs,
            (unsigned)status.last_playback_id,
            status.last_result);
        return send_data(req, data);
    }

    static esp_err_t mode_get_handler(httpd_req_t *req)
    {
        system_status_snapshot_t state;
        system_status_get_snapshot(&state);
        char data[160];
        snprintf(
            data,
            sizeof(data),
            "{\"mode\":\"%s\",\"source\":\"%s\",\"physical_switch_available\":%s}",
            state.mode,
            state.mode_source,
            button_manager_mode_switch_available() ? "true" : "false");
        return send_data(req, data);
    }

    static esp_err_t mode_post_handler(httpd_req_t *req)
    {
        int error_status = 400;
        cJSON *json = parse_json_body(req, HTTP_API_MAX_JSON_BODY, &error_status);
        if (json == NULL) {
            return send_error(req, error_status, "INVALID_ARGUMENT", "Expected a valid JSON mode object");
        }

        const cJSON *mode_item = cJSON_GetObjectItemCaseSensitive(json, "mode");
        if (!cJSON_IsString(mode_item) || mode_item->valuestring == NULL) {
            cJSON_Delete(json);
            return send_error(req, 400, "INVALID_ARGUMENT", "Mode must be a string");
        }

        const char *mode = mode_item->valuestring;
        if (strcmp(mode, "vibration_only") != 0 && strcmp(mode, "vibration_sound") != 0) {
            cJSON_Delete(json);
            return send_error(req, 400, "INVALID_ARGUMENT", "Supported modes are vibration_only and vibration_sound");
        }
        if (button_manager_mode_switch_available()) {
            cJSON_Delete(json);
            return send_error(req, 409, "PHYSICAL_SWITCH_AUTHORITATIVE", "Mode is controlled by the physical switch");
        }

        system_status_set_mode(mode, "software");
        cJSON_Delete(json);
        char data[96];
        snprintf(data, sizeof(data), "{\"mode\":\"%s\",\"source\":\"software\"}", mode);
        return send_data(req, data);
    }

    static bool valid_haptic_pattern(const char *pattern)
    {
        static const char *const patterns[] = {
            "left_pulse", "right_pulse", "both_pulse", "left_double_pulse",
            "right_double_pulse", "warning_both", "stop",
        };
        for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); ++i) {
            if (strcmp(pattern, patterns[i]) == 0) {
                return true;
            }
        }
        return false;
    }

    static esp_err_t haptics_pattern_handler(httpd_req_t *req)
    {
        int error_status = 400;
        cJSON *json = parse_json_body(req, HTTP_API_MAX_JSON_BODY, &error_status);
        if (json == NULL) {
            return send_error(req, error_status, "INVALID_ARGUMENT", "Expected a valid JSON pattern object");
        }
        const cJSON *pattern_item = cJSON_GetObjectItemCaseSensitive(json, "pattern");
        if (!cJSON_IsString(pattern_item) || pattern_item->valuestring == NULL ||
            !valid_haptic_pattern(pattern_item->valuestring)) {
            cJSON_Delete(json);
            return send_error(req, 400, "INVALID_ARGUMENT", "Pattern name is not supported");
        }

        const esp_err_t err = haptics_start_pattern(pattern_item->valuestring);
        cJSON_Delete(json);
        if (err == ESP_ERR_NOT_SUPPORTED) {
            event_manager_record(EVENT_HAPTICS_REJECTED);
            return send_error(req, 503, "HAPTICS_NOT_CONFIGURED", "Motor driver GPIOs are not configured");
        }
        if (err != ESP_OK) {
            return send_error(req, 500, "INTERNAL_ERROR", "Haptic pattern could not be started");
        }
        return send_data(req, "{\"accepted\":true}");
    }

    static esp_err_t haptics_status_handler(httpd_req_t *req)
    {
        haptics_status_t status;
        haptics_get_status(&status);
        char data[192];
        snprintf(
            data,
            sizeof(data),
            "{\"configured\":%s,\"left_active\":%s,\"right_active\":%s,\"pattern\":\"%s\"}",
            status.configured ? "true" : "false",
            status.left_active ? "true" : "false",
            status.right_active ? "true" : "false",
            status.pattern);
        return send_data(req, data);
    }

    static esp_err_t haptics_config_get_handler(httpd_req_t *req)
    {
        haptics_status_t status;
        haptics_get_status(&status);
        char data[128];
        snprintf(
            data,
            sizeof(data),
            "{\"configured\":%s,\"enabled\":%s,\"pulse_ms\":null}",
            status.configured ? "true" : "false",
            status.configured ? "true" : "false");
        return send_data(req, data);
    }

    static esp_err_t haptics_config_put_handler(httpd_req_t *req)
    {
        (void)req;
        return send_error(req, 503, "HAPTICS_NOT_CONFIGURED", "Motor driver GPIOs are not configured");
    }

    static esp_err_t haptics_test_handler(httpd_req_t *req)
    {
        (void)req;
        return send_error(req, 503, "HAPTICS_NOT_CONFIGURED", "Motor driver GPIOs are not configured");
    }

    static esp_err_t haptics_alert_handler(httpd_req_t *req)
    {
        (void)req;
        return send_error(req, 503, "HAPTICS_NOT_CONFIGURED", "Motor driver GPIOs are not configured");
    }

    static esp_err_t command_handler(httpd_req_t *req)
    {
        int error_status = 400;
        cJSON *json = parse_json_body(req, HTTP_API_MAX_JSON_BODY, &error_status);
        if (json == NULL) {
            return send_error(req, error_status, "INVALID_ARGUMENT", "Expected a valid JSON command object");
        }

        const cJSON *command_item = cJSON_GetObjectItemCaseSensitive(json, "command");
        if (!cJSON_IsString(command_item) || command_item->valuestring == NULL) {
            cJSON_Delete(json);
            return send_error(req, 400, "INVALID_ARGUMENT", "Command must be a string");
        }

        const char *command = command_item->valuestring;
        if (strcmp(command, "get_status") == 0) {
            cJSON_Delete(json);
            return status_handler(req);
        }
        if (strcmp(command, "stop_audio") == 0) {
            const esp_err_t err = audio_player_stop();
            cJSON_Delete(json);
            return err == ESP_ERR_NOT_SUPPORTED
                ? send_error(req, 503, "AUDIO_NOT_CONFIGURED", "Audio output is not configured")
                : err == ESP_OK
                    ? send_data(req, "{\"stopped\":true}")
                    : send_error(req, 500, "INTERNAL_ERROR", "Audio stop failed");
        }
        if (strcmp(command, "stop_haptics") == 0) {
            const esp_err_t err = haptics_stop();
            cJSON_Delete(json);
            return err == ESP_ERR_NOT_SUPPORTED
                ? send_error(req, 503, "HAPTICS_NOT_CONFIGURED", "Motor driver GPIOs are not configured")
                : err == ESP_OK
                    ? send_data(req, "{\"stopped\":true}")
                    : send_error(req, 500, "INTERNAL_ERROR", "Haptic stop failed");
        }

        cJSON_Delete(json);
        return send_error(req, 400, "INVALID_ARGUMENT", "Command is not supported");
    }

    static esp_err_t capabilities_handler(httpd_req_t *req)
    {
        system_status_snapshot_t state;
        wifi_manager_status_t wifi;
        system_status_get_snapshot(&state);
        wifi_manager_get_status(&wifi);
        char data[384];
        snprintf(
            data,
            sizeof(data),
            "{\"camera\":%s,\"wifi_access_point\":%s,\"audio_upload\":false,\"audio_playback\":false,"
            "\"haptics\":false,\"battery_monitoring\":false,\"distance_sensors\":false,"
            "\"physical_mode_switch\":%s,\"software_mode_control\":%s}",
            state.camera_initialized ? "true" : "false",
            wifi.active ? "true" : "false",
            button_manager_mode_switch_available() ? "true" : "false",
            button_manager_mode_switch_available() ? "false" : "true");
        return send_data(req, data);
    }

    static esp_err_t metrics_handler(httpd_req_t *req)
    {
        system_status_snapshot_t state;
        system_status_get_snapshot(&state);
        char data[256];
        snprintf(
            data,
            sizeof(data),
            "{\"camera\":{\"frames_captured\":%lu,\"frames_failed\":%lu,\"capture_latency_ms\":%lu},"
            "\"memory\":{\"free_heap_bytes\":%lu,\"minimum_free_heap_bytes\":%lu,\"largest_free_heap_block\":%lu}}",
            (unsigned long)state.frames_captured,
            (unsigned long)state.frames_failed,
            (unsigned long)state.capture_latency_ms,
            (unsigned long)state.free_heap_bytes,
            (unsigned long)state.minimum_free_heap_bytes,
            (unsigned long)state.largest_free_heap_block);
        return send_data(req, data);
    }

    static esp_err_t events_handler(httpd_req_t *req)
    {
        char events[EVENT_MANAGER_CAPACITY][EVENT_MANAGER_NAME_MAX];
        const size_t count = event_manager_copy(events, EVENT_MANAGER_CAPACITY);
        char data[768] = "{\"events\":[";
        size_t offset = strlen(data);
        for (size_t i = 0; i < count; ++i) {
            const int written = snprintf(
                data + offset,
                sizeof(data) - offset,
                "%s\"%s\"",
                i == 0 ? "" : ",",
                events[i]);
            if (written < 0 || (size_t)written >= sizeof(data) - offset) {
                return send_error(req, 500, "INTERNAL_ERROR", "Event response could not be encoded");
            }
            offset += (size_t)written;
        }
        snprintf(data + offset, sizeof(data) - offset, "]}");
        return send_data(req, data);
    }

    static esp_err_t not_found_handler(httpd_req_t *req)
    {
        static const char *const known_routes[] = {
            "/health",
            "/api/v1/status",
            "/api/v1/frame",
            "/api/v1/sensors",
            "/api/v1/audio",
            "/api/v1/audio/status",
            "/api/v1/mode",
            "/api/v1/haptics/pattern",
            "/api/v1/haptics/status",
            "/api/v1/haptics/config",
            "/api/v1/haptics/test",
            "/api/v1/haptics/alert",
            "/api/v1/command",
            "/api/v1/capabilities",
            "/api/v1/metrics",
            "/api/v1/events",
        };
        for (size_t i = 0; i < sizeof(known_routes) / sizeof(known_routes[0]); ++i) {
            if (strcmp(req->uri, known_routes[i]) == 0) {
                return send_error(req, 405, "METHOD_NOT_ALLOWED", "HTTP method is not supported for this route");
            }
        }
        return send_error(req, 404, "NOT_FOUND", "API route was not found");
    }

    static const httpd_uri_t s_routes[] = {
        {.uri = "/health", .method = HTTP_GET, .handler = health_handler},
        {.uri = "/api/v1/status", .method = HTTP_GET, .handler = status_handler},
        {.uri = "/api/v1/frame", .method = HTTP_GET, .handler = frame_handler},
        {.uri = "/api/v1/sensors", .method = HTTP_GET, .handler = sensors_handler},
        {.uri = "/api/v1/audio", .method = HTTP_POST, .handler = audio_handler},
        {.uri = "/api/v1/audio/status", .method = HTTP_GET, .handler = audio_status_handler},
        {.uri = "/api/v1/mode", .method = HTTP_GET, .handler = mode_get_handler},
        {.uri = "/api/v1/mode", .method = HTTP_POST, .handler = mode_post_handler},
        {.uri = "/api/v1/haptics/pattern", .method = HTTP_POST, .handler = haptics_pattern_handler},
        {.uri = "/api/v1/haptics/status", .method = HTTP_GET, .handler = haptics_status_handler},
        {.uri = "/api/v1/haptics/config", .method = HTTP_GET, .handler = haptics_config_get_handler},
        {.uri = "/api/v1/haptics/config", .method = HTTP_PUT, .handler = haptics_config_put_handler},
        {.uri = "/api/v1/haptics/test", .method = HTTP_POST, .handler = haptics_test_handler},
        {.uri = "/api/v1/haptics/alert", .method = HTTP_POST, .handler = haptics_alert_handler},
        {.uri = "/api/v1/command", .method = HTTP_POST, .handler = command_handler},
        {.uri = "/api/v1/capabilities", .method = HTTP_GET, .handler = capabilities_handler},
        {.uri = "/api/v1/metrics", .method = HTTP_GET, .handler = metrics_handler},
        {.uri = "/api/v1/events", .method = HTTP_GET, .handler = events_handler},
        {.uri = "/*", .method = HTTP_ANY, .handler = not_found_handler},
    };

    esp_err_t http_api_start(void)
    {
        if (s_server != NULL) {
            return ESP_ERR_INVALID_STATE;
        }

        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = 80;
        config.max_open_sockets = 4;
        config.max_uri_handlers = sizeof(s_routes) / sizeof(s_routes[0]);
        config.lru_purge_enable = true;
        config.recv_wait_timeout = 5;
        config.send_wait_timeout = 5;
        config.uri_match_fn = httpd_uri_match_wildcard;

        esp_err_t err = httpd_start(&s_server, &config);
        if (err != ESP_OK) {
            return err;
        }

        for (size_t i = 0; i < sizeof(s_routes) / sizeof(s_routes[0]); ++i) {
            err = httpd_register_uri_handler(s_server, &s_routes[i]);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to register route %s: %s", s_routes[i].uri, esp_err_to_name(err));
                httpd_stop(s_server);
                s_server = NULL;
                return err;
            }
        }

        ESP_LOGI(TAG, "HTTP API listening on port 80");
        return ESP_OK;
    }
