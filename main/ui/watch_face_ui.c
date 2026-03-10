/**
 * @file watch_face_ui.c
 * @brief Watch Face UI Implementation
 */

#include "watch_face_ui.h"
#include "board_config.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "WATCH_FACE";

static struct {
    lv_obj_t *screen;
    lv_obj_t *time_label;
    lv_obj_t *date_label;
    lv_obj_t *battery_label;
    lv_obj_t *battery_bar;
} s_watch_face = {0};

void watch_face_ui_init(void) {
    ESP_LOGI(TAG, "Initializing watch face UI...");
    
    // Create screen
    s_watch_face.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_watch_face.screen, lv_color_black(), 0);
    
    // Time label (large font, center)
    s_watch_face.time_label = lv_label_create(s_watch_face.screen);
    lv_label_set_text(s_watch_face.time_label, "00:00");
    lv_obj_set_style_text_color(s_watch_face.time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_watch_face.time_label, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_font(s_watch_face.time_label, lv_theme_get_font_large(NULL), 0);
    lv_obj_align(s_watch_face.time_label, LV_ALIGN_CENTER, 0, -40);
    
    // Date label
    s_watch_face.date_label = lv_label_create(s_watch_face.screen);
    lv_label_set_text(s_watch_face.date_label, "2026-01-01");
    lv_obj_set_style_text_color(s_watch_face.date_label, lv_color_white(), 0);
    lv_obj_align(s_watch_face.date_label, LV_ALIGN_CENTER, 0, 10);
    
    // Battery icon/label
    s_watch_face.battery_label = lv_label_create(s_watch_face.screen);
    lv_label_set_text(s_watch_face.battery_label, "BAT: 100%");
    lv_obj_set_style_text_color(s_watch_face.battery_label, lv_color_white(), 0);
    lv_obj_align(s_watch_face.battery_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    
    // Battery bar
    s_watch_face.battery_bar = lv_bar_create(s_watch_face.screen);
    lv_bar_set_range(s_watch_face.battery_bar, 0, 100);
    lv_bar_set_value(s_watch_face.battery_bar, 100, LV_ANIM_OFF);
    lv_obj_set_size(s_watch_face.battery_bar, 80, 10);
    lv_obj_align(s_watch_face.battery_bar, LV_ALIGN_BOTTOM_RIGHT, -10, -35);
    lv_obj_set_style_bg_color(s_watch_face.battery_bar, lv_color_make(0, 255, 0), LV_PART_INDICATOR);
    
    ESP_LOGI(TAG, "Watch face UI initialized");
}

void watch_face_ui_update_time(uint8_t hour, uint8_t minute, uint8_t second) {
    if (!s_watch_face.time_label) return;
    
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, minute);
    lv_label_set_text(s_watch_face.time_label, time_str);
    
    // Blink colon every second
    if (second % 2 == 0) {
        time_str[2] = ':';
    } else {
        time_str[2] = ' ';
    }
}

void watch_face_ui_update_date(uint16_t year, uint8_t month, uint8_t day) {
    if (!s_watch_face.date_label) return;
    
    char date_str[16];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", year, month, day);
    lv_label_set_text(s_watch_face.date_label, date_str);
}

void watch_face_ui_update_battery(uint8_t percent, uint16_t voltage_mv) {
    if (!s_watch_face.battery_label || !s_watch_face.battery_bar) return;
    
    char batt_str[16];
    snprintf(batt_str, sizeof(batt_str), "🔋 %d%%", percent);
    lv_label_set_text(s_watch_face.battery_label, batt_str);
    
    lv_bar_set_value(s_watch_face.battery_bar, percent, LV_ANIM_ON);
    
    // Change color based on level
    lv_color_t color;
    if (percent > 50) {
        color = lv_color_make(0, 255, 0);  // Green
    } else if (percent > 20) {
        color = lv_color_make(255, 165, 0);  // Orange
    } else {
        color = lv_color_make(255, 0, 0);  // Red
    }
    lv_obj_set_style_bg_color(s_watch_face.battery_bar, color, LV_PART_INDICATOR);
    
    ESP_LOGD(TAG, "Battery updated: %d%% (%d mV)", percent, voltage_mv);
}

lv_obj_t* watch_face_ui_get_screen(void) {
    return s_watch_face.screen;
}
