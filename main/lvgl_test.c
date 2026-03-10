/**
 * @file lvgl_test.c
 * @brief Rich LVGL Test UI
 */

#include "lvgl_test.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "lvgl.h"
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

static const char *TAG = "LVGL_TEST";

static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *battery_bar = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *status_label = NULL;

// Update time every second
static void update_time(void) {
    if (!time_label || !date_label) return;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char time_str[32];
    char date_str[32];
    
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d %s",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             (char *[]){"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}[timeinfo.tm_wday]);
    
    lv_label_set_text(time_label, time_str);
    lv_label_set_text(date_label, date_str);
}

// Timer callback
static void timer_cb(lv_timer_t *timer) {
    update_time();
    
    // Animate battery bar
    static int8_t battery_level = 75;
    static int8_t direction = -1;
    battery_level += direction;
    if (battery_level <= 20 || battery_level >= 100) direction = -direction;
    
    lv_bar_set_value(battery_bar, battery_level, LV_ANIM_ON);
    
    char batt_str[16];
    snprintf(batt_str, sizeof(batt_str), "🔋 %d%%", battery_level);
    lv_label_set_text(battery_label, batt_str);
    
    // Update status
    static uint32_t counter = 0;
    counter++;
    char status_str[64];
    snprintf(status_str, sizeof(status_str), "Uptime: %" PRIu32 " s | Heap: %" PRIu32 " KB", 
             counter, (uint32_t)(esp_get_free_heap_size() / 1024));
    lv_label_set_text(status_label, status_str);
}

esp_err_t lvgl_test_init(void) {
    ESP_LOGI(TAG, "LVGL Test Init");
    return ESP_OK;
}

esp_err_t lvgl_test_run(void) {
    ESP_LOGI(TAG, "=== LVGL Rich Test Starting ===");
    
    // Create main screen with gradient-like background
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), 0);  // Dark blue
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_scr_load(scr);
    
    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32-S3 Watch");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Time label (large)
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, LV_FONT_DEFAULT, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -30);
    
    // Date label
    date_label = lv_label_create(scr);
    lv_label_set_text(date_label, "2026-03-10 Tue");
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xa0a0a0), 0);
    lv_obj_set_style_text_font(date_label, LV_FONT_DEFAULT, 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 20);
    
    // Battery section
    lv_obj_t *battery_container = lv_obj_create(scr);
    lv_obj_set_size(battery_container, 200, 60);
    lv_obj_align(battery_container, LV_ALIGN_CENTER, 0, 70);
    lv_obj_set_style_bg_color(battery_container, lv_color_hex(0x2d2d44), 0);
    lv_obj_set_style_radius(battery_container, 10, 0);
    
    battery_bar = lv_bar_create(battery_container);
    lv_obj_set_size(battery_bar, 180, 20);
    lv_obj_align(battery_bar, LV_ALIGN_TOP_MID, 0, 5);
    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, 75, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x00ff00), LV_PART_INDICATOR);
    
    battery_label = lv_label_create(battery_container);
    lv_label_set_text(battery_label, "🔋 75%");
    lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);
    lv_obj_align(battery_label, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // Status label
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Initializing...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x808080), 0);
    lv_obj_set_style_text_font(status_label, LV_FONT_DEFAULT, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Create buttons
    lv_obj_t *btn1 = lv_btn_create(scr);
    lv_obj_set_size(btn1, 100, 35);
    lv_obj_align(btn1, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0x00aa00), 0);
    
    lv_obj_t *btn1_label = lv_label_create(btn1);
    lv_label_set_text(btn1_label, "Settings");
    lv_obj_center(btn1_label);
    
    lv_obj_t *btn2 = lv_btn_create(scr);
    lv_obj_set_size(btn2, 100, 35);
    lv_obj_align(btn2, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0xaa0000), 0);
    
    lv_obj_t *btn2_label = lv_label_create(btn2);
    lv_label_set_text(btn2_label, "Menu");
    lv_obj_center(btn2_label);
    
    // Create arc (progress indicator)
    lv_obj_t *arc = lv_arc_create(scr);
    lv_obj_set_size(arc, 80, 80);
    lv_obj_align(arc, LV_ALIGN_TOP_RIGHT, -10, 50);
    lv_arc_set_value(arc, 65);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0xff6600), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
    
    lv_obj_t *arc_label = lv_label_create(arc);
    lv_label_set_text(arc_label, "65%");
    lv_obj_center(arc_label);
    
    ESP_LOGI(TAG, "LVGL rich test screen created");
    ESP_LOGI(TAG, "Screen size: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    // Start timer (1 second)
    lv_timer_create(timer_cb, 1000, NULL);
    
    // Initial update
    update_time();
    
    return ESP_OK;
}
