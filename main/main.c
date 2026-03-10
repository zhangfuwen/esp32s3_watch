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
#include "display_test.h"
#include "simple_test.h"
#include "test_menu.h"
#include "touch_driver.h"
#include "battery_driver.h"
#include "motion_detect.h"
#include "watch_face_ui.h"
#include "ble_notify.h"
#include "time_update.h"
#include "lvgl_port.h"
#include "lvgl_test.h"

// Button GPIO
#define BOOT_BUTTON_GPIO  GPIO_NUM_0

static const char *TAG = "WATCH";

static lv_color_t lcd_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 10] __attribute__((aligned(4)));

static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2 + 1;
    int32_t y2 = area->y2 + 1;
    
    uint32_t len = (x2 - x1) * (y2 - y1);

    for (uint32_t i = 0; i < len; i++) {
        display_draw_pixel(x1 + (i % (x2 - x1)), y1 + (i / (x2 - x1)), color_p->full);
        color_p++;
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
    static lv_disp_draw_buf_t disp_buf;
    
    lv_disp_drv_init(&disp_drv);
    
    // Initialize draw buffer
    lv_disp_draw_buf_init(&disp_buf, lcd_buffer, NULL, DISPLAY_WIDTH * DISPLAY_HEIGHT / 10);

    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.wait_cb = lvgl_wait_cb;
    disp_drv.draw_buf = &disp_buf;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to register LVGL display driver");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LVGL display registered: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return ESP_OK;
}

/**
 * @brief BLE notification callback
 */
static void ble_notify_callback(const char *message, uint8_t len) {
    ESP_LOGI(TAG, "BLE notification received: %.*s", len, message);
    // Show notification on watch face or trigger vibration
    power_manager_user_activity();
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
    
    // Initialize I2C first (for touch and battery)
    ESP_LOGI(TAG, "Initializing I2C...");
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    ESP_LOGI(TAG, "I2C initialized");
    
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
    
    // Initialize motion detection
    ESP_LOGI(TAG, "Initializing motion detection...");
    motion_detect_init();
    
    // Initialize LVGL system (includes display init)
    ESP_LOGI(TAG, "Initializing LVGL system...");
    esp_err_t lvgl_ret = lvgl_init_system();
    if (lvgl_ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL init failed: %s", esp_err_to_name(lvgl_ret));
    } else {
        ESP_LOGI(TAG, "LVGL system initialized");
        
        // Start LVGL tasks
        lvgl_start_tasks();
        
        // Run LVGL test
        lvgl_test_run();
    }
    
    ESP_LOGI(TAG, "=== Hardware Init Complete ===");
    ESP_LOGI(TAG, "Press BOOT button (GPIO0) to run simple display test");
    
    return ESP_OK;
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32-S3 Watch Starting ===");
    ESP_LOGI(TAG, "Version: 1.9.0 (LVGL Test)");
    ESP_LOGI(TAG, "Build Date: %s %s", __DATE__, __TIME__);
    
    // Initialize hardware
    ESP_ERROR_CHECK(init_hardware());
    
    // Configure BOOT button as input
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);
    
    ESP_LOGI(TAG, "=== System Ready ===");
    ESP_LOGI(TAG, "LVGL test screen should be visible");
    ESP_LOGI(TAG, "Features: Large Font + Wrist Wake + Auto Sleep");
    ESP_LOGI(TAG, "Press BOOT button or raise wrist to wake...");
    
    // Main loop - check buttons and motion
    while (1) {
        // Check BOOT button (manual wake)
        if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce
            if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
                ESP_LOGI(TAG, "BOOT button pressed!");
                lvgl_test_user_activity();
                vTaskDelay(pdMS_TO_TICKS(200));  // Wait for release
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
