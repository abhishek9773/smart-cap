#include "haptics.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#ifndef CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE
#define CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE 0
#endif

#ifndef CONFIG_SMARTCAP_HAPTIC_ACTIVE_HIGH
#define CONFIG_SMARTCAP_HAPTIC_ACTIVE_HIGH 1
#endif

#define HAPTICS_TEST_TASK_STACK_SIZE 3072
#define HAPTICS_TEST_TASK_PRIORITY 3

static const char *TAG = "haptics";
static portMUX_TYPE s_state_lock = portMUX_INITIALIZER_UNLOCKED;

#if CONFIG_SMARTCAP_HAPTIC_ACTIVE_HIGH
#define HAPTICS_ACTIVE_LEVEL 1
#define HAPTICS_INACTIVE_LEVEL 0
#else
#define HAPTICS_ACTIVE_LEVEL 0
#define HAPTICS_INACTIVE_LEVEL 1
#endif

#if CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE
static TaskHandle_t s_test_task;
static bool s_test_enabled;
#endif

static bool s_initialized;
static bool s_left_active;
static bool s_right_active;
static const char *s_pattern = "idle";

#if CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE
static bool motor_test_is_enabled(void)
{
    portENTER_CRITICAL(&s_state_lock);
    const bool enabled = s_test_enabled;
    portEXIT_CRITICAL(&s_state_lock);
    return enabled;
}

static void set_motor_test_enabled(bool enabled)
{
    portENTER_CRITICAL(&s_state_lock);
    s_test_enabled = enabled;
    portEXIT_CRITICAL(&s_state_lock);
}

static void set_pattern(const char *pattern)
{
    portENTER_CRITICAL(&s_state_lock);
    s_pattern = pattern;
    portEXIT_CRITICAL(&s_state_lock);
}

static esp_err_t set_outputs(bool left_active, bool right_active)
{
    const int left_level = left_active ? HAPTICS_ACTIVE_LEVEL : HAPTICS_INACTIVE_LEVEL;
    const int right_level = right_active ? HAPTICS_ACTIVE_LEVEL : HAPTICS_INACTIVE_LEVEL;

    esp_err_t err = gpio_set_level(HAPTICS_LEFT_GPIO, left_level);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO15: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_set_level(HAPTICS_RIGHT_GPIO, right_level);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO16: %s", esp_err_to_name(err));
        gpio_set_level(HAPTICS_LEFT_GPIO, HAPTICS_INACTIVE_LEVEL);
        gpio_set_level(HAPTICS_RIGHT_GPIO, HAPTICS_INACTIVE_LEVEL);
        return err;
    }

    portENTER_CRITICAL(&s_state_lock);
    s_left_active = left_active;
    s_right_active = right_active;
    portEXIT_CRITICAL(&s_state_lock);
    return ESP_OK;
}

static bool wait_or_stop(TickType_t duration)
{
    (void)ulTaskNotifyTake(pdTRUE, duration);
    return motor_test_is_enabled();
}

static bool run_motor_step(const char *label, bool left, bool right)
{
    ESP_LOGI(TAG, "%s", label);
    if (set_outputs(left, right) != ESP_OK) {
        set_motor_test_enabled(false);
        return false;
    }

    const bool continue_test = wait_or_stop(pdMS_TO_TICKS(1000));
    const esp_err_t off_err = set_outputs(false, false);
    if (off_err != ESP_OK) {
        set_motor_test_enabled(false);
        return false;
    }
    return continue_test;
}

static void motor_test_task(void *arg)
{
    (void)arg;

    while (true) {
        if (!motor_test_is_enabled()) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        set_pattern("left_pulse");
        if (!run_motor_step("LEFT MOTOR TEST", true, false) ||
            !wait_or_stop(pdMS_TO_TICKS(1000))) {
            set_outputs(false, false);
            set_pattern("idle");
            continue;
        }

        set_pattern("right_pulse");
        if (!run_motor_step("RIGHT MOTOR TEST", false, true) ||
            !wait_or_stop(pdMS_TO_TICKS(1000))) {
            set_outputs(false, false);
            set_pattern("idle");
            continue;
        }

        set_pattern("both_pulse");
        if (!run_motor_step("BOTH MOTORS TEST", true, true) ||
            !wait_or_stop(pdMS_TO_TICKS(2000))) {
            set_outputs(false, false);
            set_pattern("idle");
            continue;
        }
        set_pattern("idle");
    }
}
#endif

esp_err_t haptics_init(void)
{
#if !CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE
    ESP_LOGI(TAG, "Motor test is disabled; GPIO15/GPIO16 remain untouched");
    return ESP_OK;
#else
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << HAPTICS_LEFT_GPIO) | (1ULL << HAPTICS_RIGHT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_set_level(HAPTICS_LEFT_GPIO, HAPTICS_INACTIVE_LEVEL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to preload inactive level on GPIO15: %s", esp_err_to_name(err));
        return err;
    }
    err = gpio_set_level(HAPTICS_RIGHT_GPIO, HAPTICS_INACTIVE_LEVEL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to preload inactive level on GPIO16: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_config(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure motor outputs GPIO15/GPIO16: %s", esp_err_to_name(err));
        return err;
    }

    err = set_outputs(false, false);
    if (err != ESP_OK) {
        return err;
    }

    portENTER_CRITICAL(&s_state_lock);
    s_initialized = true;
    portEXIT_CRITICAL(&s_state_lock);
    ESP_LOGI(
        TAG,
        "Motor test GPIOs initialized (left=GPIO15, right=GPIO16, active_%s)",
        CONFIG_SMARTCAP_HAPTIC_ACTIVE_HIGH ? "high" : "low");
    return ESP_OK;
#endif
}

esp_err_t haptics_start_motor_test(void)
{
#if !CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE
    ESP_LOGW(TAG, "Motor test start requested while CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE is disabled");
    return ESP_ERR_NOT_SUPPORTED;
#else
    portENTER_CRITICAL(&s_state_lock);
    const bool initialized = s_initialized;
    portEXIT_CRITICAL(&s_state_lock);
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_test_task == NULL) {
        set_motor_test_enabled(true);
        const BaseType_t result = xTaskCreate(
            motor_test_task,
            "haptics_test",
            HAPTICS_TEST_TASK_STACK_SIZE,
            NULL,
            HAPTICS_TEST_TASK_PRIORITY,
            &s_test_task);
        if (result != pdPASS) {
            set_motor_test_enabled(false);
            s_test_task = NULL;
            ESP_LOGE(TAG, "Failed to create motor test task");
            return ESP_ERR_NO_MEM;
        }
    } else {
        set_motor_test_enabled(true);
        xTaskNotifyGive(s_test_task);
    }
    return ESP_OK;
#endif
}

void haptics_get_status(haptics_status_t *status)
{
    if (status == NULL) {
        return;
    }

    portENTER_CRITICAL(&s_state_lock);
    status->configured = s_initialized;
    status->left_active = s_left_active;
    status->right_active = s_right_active;
    status->pattern = s_pattern;
    portEXIT_CRITICAL(&s_state_lock);
}

esp_err_t haptics_start_pattern(const char *pattern)
{
    (void)pattern;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t haptics_stop(void)
{
#if !CONFIG_SMARTCAP_HAPTIC_TEST_ENABLE
    return ESP_ERR_NOT_SUPPORTED;
#else
    portENTER_CRITICAL(&s_state_lock);
    const bool initialized = s_initialized;
    portEXIT_CRITICAL(&s_state_lock);
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    set_motor_test_enabled(false);
    set_pattern("idle");
    if (s_test_task != NULL) {
        xTaskNotifyGive(s_test_task);
    } else {
        return set_outputs(false, false);
    }
    return ESP_OK;
#endif
}
