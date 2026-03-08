/**
 * @file power_manager.c
 * @brief 电源管理实现 - 超长续航核心
 */

#include "power_manager.h"
#include "event_bus.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "soc/soc.h"
#include <sys/time.h>

static const char *TAG = "POWER_MGR";

static power_mode_t s_current_mode = POWER_MODE_ACTIVE;
static uint32_t s_idle_timeout_ms = 30000;
static uint64_t s_last_activity_time = 0;
static bool s_wakeup_sources_configured = false;

#define WAKEUP_SOURCE_TIMER    (1 << 0)
#define WAKEUP_SOURCE_BUTTON  (1 << 1)
#define WAKEUP_SOURCE_IMU     (1 << 2)
#define WAKEUP_SOURCE_TOUCH   (1 << 3)
#define WAKEUP_SOURCE_BLE     (1 << 4)
#define WAKEUP_SOURCE_RTC     (1 << 5)

static uint32_t s_enabled_wakeup_sources = WAKEUP_SOURCE_TIMER | WAKEUP_SOURCE_BUTTON | WAKEUP_SOURCE_IMU;

static uint32_t get_idle_timeout(void)
{
    switch (s_current_mode) {
        case POWER_MODE_ACTIVE:
            return 10000;    // 10s in active mode
        case POWER_MODE_IDLE:
            return 30000;    // 30s in idle mode
        case POWER_MODE_SLEEP:
            return 60000;   // 1min in sleep mode
        case POWER_MODE_DEEP_SLEEP:
            return 0;       // No auto-wake in deep sleep
        default:
            return 30000;
    }
}

static void update_power_mode(power_mode_t new_mode)
{
    if (s_current_mode == new_mode) {
        return;
    }

    power_mode_t old_mode = s_current_mode;
    s_current_mode = new_mode;

    event_t event = {
        .type = EVENT_POWER_MODE_CHANGED,
        .timestamp = esp_timer_get_time() / 1000,
        .data_u32 = (uint32_t)new_mode
    };
    event_bus_publish(&event);

    ESP_LOGI(TAG, "Power mode changed: %d -> %d", old_mode, new_mode);
}

esp_err_t power_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing power manager...");

    s_current_mode = POWER_MODE_ACTIVE;
    s_last_activity_time = esp_timer_get_time() / 1000;

    // Configure wakeup buttons
    gpio_wakeup_enable(BOOT_BUTTON_GPIO, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(POWER_BUTTON_GPIO, GPIO_INTR_LOW_LEVEL);
    esp_err_t ret = esp_sleep_enable_gpio_wakeup();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "GPIO wakeup init failed: %s", esp_err_to_name(ret));
    }

    // Initialize battery ADC
    // Note: Need to add ADC pin definition in board_config.h
#ifdef CONFIG_BATTERY_ADC_GPIO
    adc_config_t adc_config = {
        .mode = ADC_READ_MODE,
        .cl div = ADC_CLOCK_DIV,
    };
    adc_init(CONFIG_BATTERY_ADC_GPIO, &adc_config);
#endif

    ESP_LOGI(TAG, "Power manager initialized");
    return ESP_OK;
}

esp_err_t power_manager_set_mode(power_mode_t mode)
{
    ESP_LOGI(TAG, "Setting power mode: %d", mode);

    switch (mode) {
        case POWER_MODE_ACTIVE:
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
            break;

        case POWER_MODE_IDLE:
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
            break;

        case POWER_MODE_SLEEP:
            // Enable timer wakeup for periodic display update
            if (s_enabled_wakeup_sources & WAKEUP_SOURCE_TIMER) {
                esp_sleep_enable_timer_wakeup(60000000); // 1 minute
            }
            break;

        case POWER_MODE_DEEP_SLEEP:
            // Configure for deep sleep with specified wakeup sources
            power_manager_enter_deep_sleep(60000000); // 1 minute default
            return ESP_OK;

        default:
            return ESP_ERR_INVALID_ARG;
    }

    update_power_mode(mode);
    return ESP_OK;
}

power_mode_t power_manager_get_mode(void)
{
    return s_current_mode;
}

esp_err_t power_manager_register_wakeup_source(uint32_t wakeup_sources)
{
    s_enabled_wakeup_sources = wakeup_sources;
    s_wakeup_sources_configured = true;

    ESP_LOGI(TAG, "Registered wakeup sources: 0x%08" PRIx32, wakeup_sources);

    // Configure ESP-IDF sleep wakeup sources
    esp_sleep_disable_all_wakeup_sources();

    if (wakeup_sources & WAKEUP_SOURCE_TIMER) {
        esp_sleep_enable_timer_wakeup(60000000); // 1 minute default
    }

    if (wakeup_sources & WAKEUP_SOURCE_BUTTON) {
        esp_sleep_enable_gpio_wakeup();
    }

    if (wakeup_sources & WAKEUP_SOURCE_IMU) {
        // IMU wakeup via external RTC GPIO
        // IMU_INT1 (GPIO 41) or IMU_INT2 (GPIO 42)
        gpio_wakeup_enable(IMU_INT1_GPIO, GPIO_INTR_HIGH_LEVEL);
        gpio_wakeup_enable(IMU_INT2_GPIO, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();
    }

    if (wakeup_sources & WAKEUP_SOURCE_TOUCH) {
        // Touch interrupt wakeup
        gpio_wakeup_enable(TOUCH_INT_GPIO, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();
    }

    return ESP_OK;
}

esp_err_t power_manager_enter_deep_sleep(uint32_t sleep_ms)
{
    ESP_LOGI(TAG, "Entering deep sleep for %" PRIu32 " ms", sleep_ms);

    // Publish sleep event
    event_t sleep_event = {
        .type = EVENT_POWER_ENTER_SLEEP,
        .timestamp = esp_timer_get_time() / 1000,
    };
    event_bus_publish(&sleep_event);

    // Disable all wakeup sources first
    esp_sleep_disable_all_wakeup_sources();

    // Configure wakeup sources for deep sleep
    if (s_enabled_wakeup_sources & WAKEUP_SOURCE_TIMER) {
        esp_sleep_enable_timer_wakeup(sleep_ms * 1000ULL); // Convert to microseconds
    }

    if (s_enabled_wakeup_sources & WAKEUP_SOURCE_BUTTON) {
        // Enable wakeup from buttons
        gpio_wakeup_enable(BOOT_BUTTON_GPIO, GPIO_INTR_LOW_LEVEL);
        gpio_wakeup_enable(POWER_BUTTON_GPIO, GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();
    }

    if (s_enabled_wakeup_sources & WAKEUP_SOURCE_IMU) {
        // IMU motion wakeup via ext1
        gpio_wakeup_enable(IMU_INT1_GPIO, GPIO_INTR_HIGH_LEVEL);
        gpio_wakeup_enable(IMU_INT2_GPIO, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_ext1_wakeup(
            (1ULL << IMU_INT1_GPIO) | (1ULL << IMU_INT2_GPIO),
            ESP_EXT1_WAKEUP_ANY_HIGH
        );
    }

    if (s_enabled_wakeup_sources & WAKEUP_SOURCE_TOUCH) {
        gpio_wakeup_enable(TOUCH_INT_GPIO, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_ext1_wakeup(1ULL << TOUCH_INT_GPIO, ESP_EXT1_WAKEUP_ANY_HIGH);
    }

    // For BLE wakeup, we need to stay in light sleep or use RTC peripheral
    if (s_enabled_wakeup_sources & WAKEUP_SOURCE_BLE) {
        // BLE can wake from light sleep, but for deep sleep we skip it
        ESP_LOGW(TAG, "BLE wakeup not supported in deep sleep, using light sleep instead");
    }

    // Enter light sleep instead of deep sleep if BLE is needed
    if (s_enabled_wakeup_sources & WAKEUP_SOURCE_BLE) {
        esp_err_t ret = esp_light_sleep_start();
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Woke up from light sleep");
        } else {
            ESP_LOGE(TAG, "Light sleep failed: %s", esp_err_to_name(ret));
        }
    } else {
        // Actual deep sleep
        esp_deep_sleep_start();
    }

    // This point is reached after wakeup
    ESP_LOGI(TAG, "Woke up from sleep");

    event_t wakeup_event = {
        .type = EVENT_POWER_WAKEUP,
        .timestamp = esp_timer_get_time() / 1000,
    };
    event_bus_publish(&wakeup_event);

    update_power_mode(POWER_MODE_ACTIVE);

    return ESP_OK;
}

uint8_t power_manager_get_battery_level(void)
{
    // Simplified battery level estimation
    // In production, read from MAX17048G fuel gauge via I2C
    
#ifdef CONFIG_BATTERY_GAUGE_I2C
    // Read from MAX17048G
    // Would need to implement I2C communication
    // For now, return simulated value
    return 75;
#else
    // Fallback: estimate from voltage if ADC is configured
    return 75;
#endif
}

bool power_manager_is_charging(void)
{
    // TODO: Add charging status pin to board_config.h
    // For now, return false
    return false;
}

void power_manager_user_activity(void)
{
    s_last_activity_time = esp_timer_get_time() / 1000;

    if (s_current_mode != POWER_MODE_ACTIVE) {
        power_manager_set_mode(POWER_MODE_ACTIVE);
    }
}
