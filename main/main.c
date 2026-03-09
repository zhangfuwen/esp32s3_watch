/**
 * ESP32-S3 Watch - Main Entry Point
 * 
 * @file main.c
 * @brief Main application entry point
 * @version 0.3
 * @date 2026-03-09
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "board_config.h"
#include "event_bus.h"
#include "power_manager.h"
#include "display_driver.h"
#include "test_menu.h"

static const char *TAG = "WATCH";

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
            
        default:
            break;
    }
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
    
    // Initialize power manager
    ESP_ERROR_CHECK(power_manager_init());
    ESP_LOGI(TAG, "Power manager initialized");
    
    // Initialize display with default config
    display_driver_config_t disp_config = {
        .spi_host = SPI2_HOST,
        .pin_mosi = DISPLAY_MOSI_PIN,
        .pin_sclk = DISPLAY_SCLK_PIN,
        .pin_cs = DISPLAY_CS_PIN,
        .pin_dc = DISPLAY_DC_PIN,
        .pin_rst = -1,  // No reset pin
        .pin_backlight = DISPLAY_BACKLIGHT_PIN,
        .spi_speed = 40000000,
        .rotation = DISPLAY_ROTATION_0,
        .invert_colors = false,
        .swap_xy = false
    };
    ESP_ERROR_CHECK(display_driver_init(&disp_config));
    ESP_LOGI(TAG, "Display driver initialized");
    
    // Initialize test menu
    test_menu_init();
    ESP_LOGI(TAG, "Test menu initialized");
    
    return ESP_OK;
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32-S3 Watch Starting ===");
    ESP_LOGI(TAG, "Version: 0.3.0 (Test Menu)");
    ESP_LOGI(TAG, "Build Date: %s %s", __DATE__, __TIME__);
    
    // Initialize hardware
    ESP_ERROR_CHECK(init_hardware());
    
    ESP_LOGI(TAG, "=== System Ready ===");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    
    // Main loop
    while (1) {
        power_manager_user_activity();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
