/**
 * @file lvgl_test.c
 * @brief Clear LVGL Test UI - High Contrast, Large Fonts
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
    snprintf(date_str, sizeof(date_str), "%d-%02d-%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    lv_label_set_text(time_label, time_str);
    lv_label_set_text(date_label, date_str);
}

// Timer callback - update every second
static void timer_cb(lv_timer_t *timer) {
    update_time();
    
    // Update status
    static uint32_t counter = 0;
    counter++;
    char status_str[64];
    snprintf(status_str, sizeof(status_str), "UP: %" PRIu32 "s | HEAP: %" PRIu32 "KB", 
             counter, (uint32_t)(esp_get_free_heap_size() / 1024));
    lv_label_set_text(status_label, status_str);
}

esp_err_t lvgl_test_init(void) {
    ESP_LOGI(TAG, "LVGL Test Init");
    return ESP_OK;
}

esp_err_t lvgl_test_run(void) {
    ESP_LOGI(TAG, "=== LVGL Clear Test Starting ===");
    
    // Create main screen with BLACK background (highest contrast)
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_scr_load(scr);
    
    // Title - WHITE, LARGE
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32-S3 WATCH");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    // Separator line
    lv_obj_t *line1 = lv_line_create(scr);
    static lv_point_t line1_points[2];
    line1_points[0].x = 10;
    line1_points[0].y = 35;
    line1_points[1].x = 230;
    line1_points[1].y = 35;
    lv_line_set_points(line1, line1_points, 2);
    lv_obj_set_style_line_color(line1, lv_color_white(), 0);
    lv_obj_set_style_line_width(line1, 2, 0);
    
    // Time label - VERY LARGE, BRIGHT YELLOW
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "12:30:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFF00), 0);  // Bright yellow
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -20);
    
    // Date label - LARGE, CYAN
    date_label = lv_label_create(scr);
    lv_label_set_text(date_label, "2026-03-10");
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x00FFFF), 0);  // Cyan
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 25);
    
    // Separator line
    lv_obj_t *line2 = lv_line_create(scr);
    static lv_point_t line2_points[2];
    line2_points[0].x = 10;
    line2_points[0].y = 100;
    line2_points[1].x = 230;
    line2_points[1].y = 100;
    lv_line_set_points(line2, line2_points, 2);
    lv_obj_set_style_line_color(line2, lv_color_white(), 0);
    lv_obj_set_style_line_width(line2, 2, 0);
    
    // Battery - LARGE, GREEN
    battery_label = lv_label_create(scr);
    lv_label_set_text(battery_label, "[BATTERY] 75%");
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0x00FF00), 0);  // Bright green
    lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 120);
    
    // Status - WHITE
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "UP: 0s | HEAP: 0KB");
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    ESP_LOGI(TAG, "LVGL clear test screen created");
    ESP_LOGI(TAG, "Screen size: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    // Start timer (1 second)
    lv_timer_create(timer_cb, 1000, NULL);
    
    // Initial update
    update_time();
    
    return ESP_OK;
}
