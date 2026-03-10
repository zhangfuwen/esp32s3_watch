/**
 * @file lvgl_test.c
 * @brief Large Font UI with Wrist Wake
 */

#include "lvgl_test.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "lvgl.h"
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

static const char *TAG = "LVGL_TEST";

static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *screen_container = NULL;
static lv_obj_t *backlight_control = NULL;

static bool display_on = false;
static uint32_t last_activity_time = 0;

// Turn display on
static void display_turn_on(void) {
    if (!display_on) {
        if (screen_container) {
            lv_obj_clear_flag(screen_container, LV_OBJ_FLAG_HIDDEN);
        }
        // Turn on backlight
        if (backlight_control) {
            lv_obj_clear_flag(backlight_control, LV_OBJ_FLAG_HIDDEN);
        }
        gpio_set_level(DISPLAY_BACKLIGHT_PIN, 1);
        display_on = true;
        last_activity_time = (uint32_t)(esp_timer_get_time() / 1000);
        ESP_LOGI(TAG, "Display ON");
    } else {
        // Already on, just update activity time
        last_activity_time = (uint32_t)(esp_timer_get_time() / 1000);
    }
}

// Turn display off
static void display_turn_off(void) {
    if (display_on) {
        if (screen_container) {
            lv_obj_add_flag(screen_container, LV_OBJ_FLAG_HIDDEN);
        }
        // Turn off backlight
        gpio_set_level(DISPLAY_BACKLIGHT_PIN, 0);
        display_on = false;
        ESP_LOGI(TAG, "Display OFF (backlight off)");
    }
}

// Activity detected (button or wrist)
void lvgl_test_user_activity(void) {
    last_activity_time = (uint32_t)(esp_timer_get_time() / 1000);
    display_turn_on();
}

// Check for auto sleep
static void check_auto_sleep(void) {
    if (!display_on) return;
    
    uint32_t current_time = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t elapsed = current_time - last_activity_time;
    
    // Auto sleep after 10 seconds
    if (elapsed > 10000) {
        display_turn_off();
    }
}

// Update time every second
static void update_time(void) {
    if (!time_label || !date_label) return;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char time_str[32];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lv_label_set_text(time_label, time_str);
    
    // Update date once per minute
    static int last_min = -1;
    if (timeinfo.tm_min != last_min) {
        last_min = timeinfo.tm_min;
        char date_str[32];
        snprintf(date_str, sizeof(date_str), "%d/%02d/%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        lv_label_set_text(date_label, date_str);
    }
}

// Timer callback - 1 second
static void timer_cb(lv_timer_t *timer) {
    update_time();
    check_auto_sleep();
    
    // Update status every 5 seconds
    static uint32_t counter = 0;
    counter++;
    if (counter % 5 == 0) {
        char status_str[64];
        snprintf(status_str, sizeof(status_str), "UP:%" PRIu32 "s H:%" PRIu32 "KB", 
                 counter, (uint32_t)(esp_get_free_heap_size() / 1024));
        lv_label_set_text(status_label, status_str);
    }
}

esp_err_t lvgl_test_init(void) {
    ESP_LOGI(TAG, "LVGL Test Init (Large Font + Wrist Wake)");
    return ESP_OK;
}

esp_err_t lvgl_test_run(void) {
    ESP_LOGI(TAG, "=== LVGL Large Font Test Starting ===");
    
    // Create main screen with BLACK background
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_scr_load(scr);
    
    // Create container for easy show/hide
    screen_container = lv_obj_create(scr);
    lv_obj_set_size(screen_container, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(screen_container, 0, 0);
    lv_obj_set_style_bg_color(screen_container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen_container, 0, 0);
    
    // Backlight is controlled by GPIO only (no LVGL object needed)
    backlight_control = NULL;
    
    // Title - WHITE, LARGE FONT
    lv_obj_t *title = lv_label_create(screen_container);
    lv_label_set_text(title, "ESP32-S3 WATCH");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    // Separator line
    lv_obj_t *line1 = lv_line_create(screen_container);
    static lv_point_t line1_points[2];
    line1_points[0].x = 10;
    line1_points[0].y = 38;
    line1_points[1].x = 230;
    line1_points[1].y = 38;
    lv_line_set_points(line1, line1_points, 2);
    lv_obj_set_style_line_color(line1, lv_color_white(), 0);
    lv_obj_set_style_line_width(line1, 2, 0);
    
    // Time label - VERY LARGE, BRIGHT YELLOW
    time_label = lv_label_create(screen_container);
    lv_label_set_text(time_label, "12:30:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -15);
    
    // Date label - LARGE, CYAN
    date_label = lv_label_create(screen_container);
    lv_label_set_text(date_label, "2026/03/10");
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_32, 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 35);
    
    // Separator line
    lv_obj_t *line2 = lv_line_create(screen_container);
    static lv_point_t line2_points[2];
    line2_points[0].x = 10;
    line2_points[0].y = 105;
    line2_points[1].x = 230;
    line2_points[1].y = 105;
    lv_line_set_points(line2, line2_points, 2);
    lv_obj_set_style_line_color(line2, lv_color_white(), 0);
    lv_obj_set_style_line_width(line2, 2, 0);
    
    // Battery - LARGE, GREEN
    lv_obj_t *battery_label = lv_label_create(screen_container);
    lv_label_set_text(battery_label, "BAT: 75%");
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_28, 0);
    lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 130);
    
    // Status - WHITE, BOTTOM
    status_label = lv_label_create(screen_container);
    lv_label_set_text(status_label, "UP:0s H:0KB");
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_24, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    // Show display on boot
    display_turn_on();
    last_activity_time = (uint32_t)(esp_timer_get_time() / 1000);
    
    ESP_LOGI(TAG, "LVGL large font screen created");
    ESP_LOGI(TAG, "Screen size: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    // Start timer (1 second)
    lv_timer_create(timer_cb, 1000, NULL);
    
    // Initial update
    update_time();
    
    return ESP_OK;
}
