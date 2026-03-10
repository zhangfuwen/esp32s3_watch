/**
 * ESP32-S3 Watch - Main Entry Point
 * 
 * @file main.c
 * @brief Main application entry point
 * @version 0.3
 * @date 2026-03-09
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "lvgl.h"
#include "esp_lcd_panel_ops.h"

#include "board_config.h"
#include "event_bus.h"
#include "power_manager.h"
#include "display_driver.h"
#include "display.h"
#include "test_menu.h"
#include "touch_driver.h"
#include "battery_driver.h"
#include "motion_detect.h"
#include "watch_face_ui.h"

static const char *TAG = "WATCH";

static uint8_t lcd_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT * 2] __attribute__((aligned(4)));

static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2 + 1;
    int32_t y2 = area->y2 + 1;

    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            display_draw_pixel(x, y, color_p->full);
            color_p++;
        }
    }

    lv_disp_flush_ready(disp_drv);
}

static void lvgl_wait_cb(lv_disp_drv_t *disp_drv)
{
}

static esp_err_t init_lvgl_display(void)
{
    ESP_LOGI(TAG, "Initializing LVGL display...");

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.wait_cb = lvgl_wait_cb;
    disp_drv.draw_buf = NULL;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to register LVGL display driver");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LVGL display registered: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return ESP_OK;
}

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
    
    // Initialize LVGL
    ESP_LOGI(TAG, "Initializing LVGL...");
    lv_init();
    ESP_LOGI(TAG, "LVGL initialized");
    
    // Initialize LVGL display driver
    if (init_lvgl_display() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL display");
    }
    
    // Initialize touch driver
    ESP_LOGI(TAG, "Initializing touch driver...");
    esp_err_t touch_ret = touch_driver_init(&g_touch_driver, I2C_NUM_0, 0x15, GPIO_NUM_41);
    if (touch_ret == ESP_OK) {
        // Register LVGL touch input device
        static lv_indev_drv_t indev_drv;
        touch_driver_register_lvgl(&indev_drv, &g_touch_driver);
    } else {
        ESP_LOGW(TAG, "Touch driver init failed: %s, continuing without touch", esp_err_to_name(touch_ret));
    }
    
    // Initialize battery driver
    ESP_LOGI(TAG, "Initializing battery driver...");
    uint8_t soc = 0;
    uint16_t voltage = 0;
    esp_err_t batt_ret = battery_driver_init(&g_battery_driver, I2C_NUM_0, 0x36);
    if (batt_ret == ESP_OK) {
        battery_driver_get_soc(&g_battery_driver, &soc);
        battery_driver_get_voltage(&g_battery_driver, &voltage);
        ESP_LOGI(TAG, "Battery: %d%%, %d mV", soc, voltage);
    } else {
        ESP_LOGW(TAG, "Battery driver init failed: %s, continuing without battery", esp_err_to_name(batt_ret));
    }
    
    // Initialize motion detection
    ESP_LOGI(TAG, "Initializing motion detection...");
    motion_detect_init();
    
    // Initialize watch face UI
    ESP_LOGI(TAG, "Initializing watch face UI...");
    watch_face_ui_init();
    watch_face_ui_update_time(12, 0, 0);
    watch_face_ui_update_date(2026, 3, 10);
    watch_face_ui_update_battery(soc, voltage);
    
    // Switch to watch face screen
    lv_scr_load(watch_face_ui_get_screen());
    ESP_LOGI(TAG, "Watch face displayed");
    
    // Initialize test menu (but don't show it)
    test_menu_init();
    ESP_LOGI(TAG, "Test menu initialized (hidden)");
    
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
    ESP_LOGI(TAG, "Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());
    
    // Main loop
    while (1) {
        power_manager_user_activity();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
