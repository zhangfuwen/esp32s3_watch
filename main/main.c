/**
 * ESP32-S3 Watch - Main Entry Point
 * 
 * @file main.c
 * @brief Main application entry point with event-driven architecture
 * @version 0.2
 * @date 2026-03-08
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_pm.h"

#include "board_config.h"
#include "event_bus.h"
#include "power_manager.h"
#include "display_driver.h"
#include "imu_driver.h"
#include "time_service.h"
#include "watch_face.h"
#include "test_menu.h"

static const char *TAG = "WATCH";
static QueueHandle_t event_queue = NULL;

// Wakeup sources
#define WAKEUP_BTN0     (1ULL << BOOT_BUTTON_GPIO)
#define WAKEUP_BTN_PWR  (1ULL << POWER_BUTTON_GPIO)
#define WAKEUP_IMU      (1ULL << 41)  // IMU INT1

/**
 * @brief Event handler for system events
 */
static void system_event_handler(const event_t *event) {
    switch (event->type) {
        case EVENT_INPUT_BUTTON_PRESS:
            ESP_LOGI(TAG, "Button pressed");
            power_manager_user_activity();
            break;
            
        case EVENT_INPUT_WRIST_TILT:
            ESP_LOGI(TAG, "Wrist raised");
            power_manager_user_activity();
            break;
            
        case EVENT_POWER_ENTER_SLEEP:
            ESP_LOGI(TAG, "Entering sleep mode");
            break;
            
        case EVENT_SYSTEM_LOW_BATTERY:
            ESP_LOGW(TAG, "Low battery warning!");
            break;
            
        default:
            break;
    }
}

/**
 * @brief Configure wakeup sources
 */
static void configure_wakeup_sources(void) {
    // Enable ext1 wakeup for buttons
    esp_sleep_enable_ext1_wakeup(WAKEUP_BTN0 | WAKEUP_BTN_PWR, ESP_EXT1_WAKEUP_ANY_HIGH);
    
    // Enable IMU as wakeup source (via INT1)
    gpio_wakeup_enable(IMU_INT1_GPIO, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    
    ESP_LOGI(TAG, "Wakeup sources configured: BOOT, PWR, IMU");
}

/**
 * @brief Initialize hardware peripherals
 */
static esp_err_t init_hardware(void) {
    esp_err_t ret;
    
    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
    
    // Initialize event bus
    ESP_ERROR_CHECK(event_bus_init());
    ESP_LOGI(TAG, "Event bus initialized");
    
    // Subscribe to system events
    event_bus_subscribe(EVENT_INPUT_BUTTON_PRESS, system_event_handler);
    event_bus_subscribe(EVENT_INPUT_WRIST_TILT, system_event_handler);
    event_bus_subscribe(EVENT_POWER_ENTER_SLEEP, system_event_handler);
    event_bus_subscribe(EVENT_SYSTEM_LOW_BATTERY, system_event_handler);
    
    // Initialize power manager
    ESP_ERROR_CHECK(power_manager_init());
    ESP_LOGI(TAG, "Power manager initialized");
    
    // Initialize display
    ESP_ERROR_CHECK(display_driver_init());
    ESP_LOGI(TAG, "Display driver initialized");
    
    // Initialize test menu
    ESP_ERROR_CHECK(test_menu_init());
    test_menu_show_main();
    ESP_LOGI(TAG, "Test menu initialized");
    
    // Initialize IMU
    ESP_ERROR_CHECK(imu_driver_init());
    ESP_LOGI(TAG, "IMU driver initialized");
    
    // Initialize time service
    time_config_t time_config = {
        .enable_ntp = true,
        .enable_ble_time = true,
        .enable_ble_notifications = true,
        .ntp_server = "pool.ntp.org",
        .timezone_hours = 8,  // CST (China Standard Time)
        .timezone_minutes = 0,
        .is_dst = false
    };
    ESP_ERROR_CHECK(time_service_init(&time_config));
    ESP_LOGI(TAG, "Time service initialized");
    
    // Configure wakeup sources
    configure_wakeup_sources();
    
    return ESP_OK;
}

/**
 * @brief Event loop task - processes system events
 */
static void event_loop_task(void *pvParameters) {
    event_queue = xQueueCreate(10, sizeof(event_t));
    if (event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Event loop started");
    
    uint32_t battery_check_counter = 0;
    
    while (1) {
        event_t event;
        BaseType_t received = xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100));
        
        if (received == pdTRUE) {
            switch (event.type) {
                case EVENT_INPUT_BUTTON_PRESS:
                    ESP_LOGI(TAG, "Event: Button press");
                    power_manager_user_activity();
                    break;
                case EVENT_INPUT_WRIST_TILT:
                    ESP_LOGI(TAG, "Event: Wrist tilt");
                    power_manager_user_activity();
                    break;
                case EVENT_POWER_ENTER_SLEEP:
                    ESP_LOGI(TAG, "Event: Enter sleep");
                    break;
                case EVENT_SYSTEM_LOW_BATTERY:
                    ESP_LOGW(TAG, "Event: Low battery (%lu%%)", event.data_u32);
                    break;
                default:
                    break;
            }
        }
        
        battery_check_counter++;
        if (battery_check_counter >= 600) {
            battery_check_counter = 0;
            uint8_t battery = power_manager_get_battery_level();
            if (battery < 15) {
                event_t low_battery_event = {
                    .type = EVENT_SYSTEM_LOW_BATTERY,
                    .data_u32 = battery
                };
                xQueueSend(event_queue, &low_battery_event, 0);
            }
        }
    }
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32-S3 Watch Starting ===");
    ESP_LOGI(TAG, "Version: 0.2.0 (Event-Driven)");
    ESP_LOGI(TAG, "Build Date: %s %s", __DATE__, __TIME__);
    
    // Check wakeup reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Wakeup by EXT1 (button)");
            break;
        case ESP_SLEEP_WAKEUP_GPIO:
            ESP_LOGI(TAG, "Wakeup by GPIO (IMU)");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wakeup by timer");
            break;
        default:
            ESP_LOGI(TAG, "Cold start (no wakeup source)");
            break;
    }
    
    // Initialize hardware
    ESP_ERROR_CHECK(init_hardware());
    
    // Initialize watch face
    watch_face_config_t wf_config = {
        .style = WATCH_FACE_DIGITAL,
        .show_seconds = true,
        .show_date = true,
        .show_battery = true,
        .show_steps = false,
        .brightness = 80
    };
    ESP_ERROR_CHECK(watch_face_init(&wf_config));
    ESP_ERROR_CHECK(watch_face_start());
    ESP_LOGI(TAG, "Watch face started");
    
    // Enable auto NTP sync
    time_service_enable_auto_sync(true);
    
    ESP_LOGI(TAG, "=== System Ready ===");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    
    // Create event loop task
    xTaskCreate(event_loop_task, "event_loop", 4096, NULL, 5, NULL);
    
    // Main task done, delete itself
    vTaskDelete(NULL);
}
