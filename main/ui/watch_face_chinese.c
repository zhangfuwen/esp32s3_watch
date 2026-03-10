/**
 * @file watch_face_chinese.c
 * @brief Chinese Watch Face with Real Chinese Font Support
 */

#include "watch_face_chinese.h"
#include "board_config.h"
#include "display.h"
#include "time_update.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "chinese_16.h"
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

static const char *TAG = "WATCH_FACE_CN";

static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *weekday_label = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *battery_bar = NULL;
static lv_obj_t *screen_container = NULL;

static bool display_on = false;
static uint32_t last_activity_time = 0;
static uint32_t start_time = 0;

// Colors
#define COLOR_BG        lv_color_hex(0x000000)
#define COLOR_TIME      lv_color_hex(0x00FF00)
#define COLOR_DATE      lv_color_hex(0x00FFFF)
#define COLOR_WEEKDAY   lv_color_hex(0xFF9900)
#define COLOR_BATTERY   lv_color_hex(0x00FF00)
#define COLOR_BAR_BG    lv_color_hex(0x333333)

void watch_face_chinese_user_activity(void) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    
    if (!display_on) {
        if (screen_container) {
            lv_obj_clear_flag(screen_container, LV_OBJ_FLAG_HIDDEN);
        }
        gpio_set_level(DISPLAY_BACKLIGHT_PIN, 1);
        display_on = true;
        ESP_LOGI(TAG, "Display ON");
    }
    
    last_activity_time = now;
}

static void display_turn_off(void) {
    if (display_on) {
        if (screen_container) {
            lv_obj_add_flag(screen_container, LV_OBJ_FLAG_HIDDEN);
        }
        gpio_set_level(DISPLAY_BACKLIGHT_PIN, 0);
        display_on = false;
        ESP_LOGI(TAG, "Display OFF");
    }
}

static void update_time(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Time: 12:30:00
    if (time_label) {
        char time_buf[20];
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", 
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        lv_label_set_text(time_label, time_buf);
    }
    
    // Date: 2026 年 03 月 10 日 (with Chinese font)
    if (date_label) {
        char date_buf[40];
        snprintf(date_buf, sizeof(date_buf), "%04d年%02d月%02d日", 
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        lv_label_set_text(date_label, date_buf);
    }
    
    // Weekday: 星期二 (with Chinese font)
    if (weekday_label) {
        const char *weekdays_cn[] = {
            "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"
        };
        lv_label_set_text(weekday_label, weekdays_cn[timeinfo.tm_wday]);
    }
}

static void update_battery(void) {
    if (battery_label && battery_bar) {
        int soc = 75;
        
        char bat_buf[20];
        snprintf(bat_buf, sizeof(bat_buf), "电量%d%%", soc);
        lv_label_set_text(battery_label, bat_buf);
        
        lv_bar_set_value(battery_bar, soc, LV_ANIM_OFF);
        
        if (soc < 20) {
            lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
        } else if (soc < 50) {
            lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0xFF9900), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(battery_bar, COLOR_BATTERY, LV_PART_INDICATOR);
        }
    }
}

static void timer_cb(lv_timer_t *timer) {
    (void)timer;
    
    update_time();
    update_battery();
    
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    if (display_on && (now - last_activity_time > 10000)) {
        display_turn_off();
    }
}

void watch_face_chinese_init(void) {
    start_time = (uint32_t)(esp_timer_get_time() / 1000);
    
    ESP_LOGI(TAG, "Creating Chinese watch face...");
    ESP_LOGI(TAG, "LVGL version: %d.%d.%d", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    ESP_LOGI(TAG, "Chinese font: chinese_16");
    ESP_LOGI(TAG, "Font line_height: %d", chinese_16.line_height);
    ESP_LOGI(TAG, "Font base_line: %d", chinese_16.base_line);
    ESP_LOGI(TAG, "Font dsc: %p", chinese_16.dsc);
    ESP_LOGI(TAG, "Font get_glyph_dsc: %p", chinese_16.get_glyph_dsc);
    ESP_LOGI(TAG, "Font get_glyph_bitmap: %p", chinese_16.get_glyph_bitmap);
    
    // Create screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_scr_load(scr);
    
    // Main container
    screen_container = lv_obj_create(scr);
    lv_obj_set_size(screen_container, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(screen_container, 0, 0);
    lv_obj_set_style_bg_opa(screen_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen_container, 0, 0);
    lv_obj_set_style_pad_all(screen_container, 0, 0);
    
    // === TIME (LARGE, Numbers only) ===
    time_label = lv_label_create(screen_container);
    lv_label_set_text(time_label, "12:30:00");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_label, COLOR_TIME, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -50);
    
    // === DATE (Chinese with chinese_16 font) ===
    date_label = lv_label_create(screen_container);
    lv_label_set_text(date_label, "2026 年 03 月 10 日");
    lv_obj_set_style_text_font(date_label, &chinese_16, 0);
    lv_obj_set_style_text_color(date_label, COLOR_DATE, 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 0);
    
    // === WEEKDAY (Chinese with chinese_16 font) ===
    weekday_label = lv_label_create(screen_container);
    lv_label_set_text(weekday_label, "星期二");
    lv_obj_set_style_text_font(weekday_label, &chinese_16, 0);
    lv_obj_set_style_text_color(weekday_label, COLOR_WEEKDAY, 0);
    lv_obj_align(weekday_label, LV_ALIGN_CENTER, 0, 25);
    
    // === BATTERY BAR ===
    battery_bar = lv_bar_create(screen_container);
    lv_obj_set_size(battery_bar, 180, 24);
    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, 75, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(battery_bar, COLOR_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(battery_bar, COLOR_BATTERY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(battery_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_align(battery_bar, LV_ALIGN_CENTER, 0, 65);
    
    // === BATTERY LABEL (Chinese with chinese_16 font) ===
    battery_label = lv_label_create(screen_container);
    lv_label_set_text(battery_label, "电量 75%");
    lv_obj_set_style_text_font(battery_label, &chinese_16, 0);
    lv_obj_set_style_text_color(battery_label, COLOR_BATTERY, 0);
    lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 95);
    
    // Show on boot
    display_on = false;
    watch_face_chinese_user_activity();
    last_activity_time = start_time;
    
    ESP_LOGI(TAG, "Chinese watch face init complete");
    ESP_LOGI(TAG, "Date label font: %p", &chinese_16);
    ESP_LOGI(TAG, "Weekday label font: %p", &chinese_16);
    ESP_LOGI(TAG, "Battery label font: %p", &chinese_16);
    
    // Start timer
    lv_timer_create(timer_cb, 1000, NULL);
    
    // Initial update
    update_time();
    update_battery();
}
