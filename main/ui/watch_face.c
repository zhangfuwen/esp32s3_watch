/**
 * @file watch_face.c
 * @brief Modern Watch Face UI
 */

#include "watch_face.h"
#include "board_config.h"
#include "display.h"
#include "battery_driver.h"
#include "time_update.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "lvgl.h"
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

static const char *TAG = "WATCH_FACE";

static lv_obj_t *hour_label = NULL;
static lv_obj_t *minute_label = NULL;
static lv_obj_t *second_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *weekday_label = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *battery_bar = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *screen_container = NULL;

static bool display_on = false;
static uint32_t last_activity_time = 0;
static uint32_t start_time = 0;

// Modern color scheme
#define COLOR_BG        lv_color_hex(0x0D1117)  // Dark blue-black
#define COLOR_HOUR      lv_color_hex(0x58A6FF)  // Bright blue
#define COLOR_MINUTE    lv_color_hex(0xF0F6FC)  // White
#define COLOR_SECOND    lv_color_hex(0xFF7B72)  // Coral red
#define COLOR_DATE      lv_color_hex(0x8B949E)  // Gray
#define COLOR_BATTERY   lv_color_hex(0x3FB950)  // Green
#define COLOR_BAR_BG    lv_color_hex(0x21262D)  // Dark gray
#define COLOR_ACCENT    lv_color_hex(0xD29922)  // Gold

// Turn display on
void watch_face_user_activity(void) {
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
    
    if (hour_label && minute_label && second_label) {
        char time_buf[20];
        
        // Format: 12:30:00
        snprintf(time_buf, sizeof(time_buf), "%02d", timeinfo.tm_hour);
        lv_label_set_text(hour_label, time_buf);
        
        snprintf(time_buf, sizeof(time_buf), "%02d", timeinfo.tm_min);
        lv_label_set_text(minute_label, time_buf);
        
        snprintf(time_buf, sizeof(time_buf), "%02d", timeinfo.tm_sec);
        lv_label_set_text(second_label, time_buf);
    }
    
    // Update date and weekday
    if (date_label && weekday_label) {
        char date_buf[40];
        const char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        
        snprintf(date_buf, sizeof(date_buf), "%02d/%02d", 
                 timeinfo.tm_mon + 1, timeinfo.tm_mday);
        lv_label_set_text(date_label, date_buf);
        
        lv_label_set_text(weekday_label, weekdays[timeinfo.tm_wday]);
    }
    
    // Update uptime
    if (status_label) {
        char status_buf[30];
        uint32_t uptime = ((uint32_t)(esp_timer_get_time() / 1000) - start_time) / 1000;
        uint32_t free_heap = esp_get_free_heap_size() / 1024;
        snprintf(status_buf, sizeof(status_buf), "UP: %"PRIu32"s  H: %"PRIu32"KB", uptime, free_heap);
        lv_label_set_text(status_label, status_buf);
    }
}

static void update_battery(void) {
    if (battery_label && battery_bar) {
        uint8_t soc = 75;  // Default
        battery_driver_get_soc(NULL, &soc);  // Use default if driver not initialized
        
        char bat_buf[20];
        snprintf(bat_buf, sizeof(bat_buf), "%d%%", soc);
        lv_label_set_text(battery_label, bat_buf);
        
        lv_bar_set_value(battery_bar, soc, LV_ANIM_OFF);
        
        // Change color based on level
        if (soc < 20) {
            lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0xFF7B72), LV_PART_INDICATOR);
        } else if (soc < 50) {
            lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0xD29922), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x3FB950), LV_PART_INDICATOR);
        }
    }
}

static void timer_cb(lv_timer_t *timer) {
    (void)timer;
    
    update_time();
    update_battery();
    
    // Auto sleep after 10 seconds
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    if (display_on && (now - last_activity_time > 10000)) {
        display_turn_off();
    }
}

void watch_face_init(void) {
    start_time = (uint32_t)(esp_timer_get_time() / 1000);
    
    // Create screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_scr_load(scr);
    
    // Main container
    screen_container = lv_obj_create(scr);
    lv_obj_set_size(screen_container, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(screen_container, 0, 0);
    lv_obj_set_style_bg_color(screen_container, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(screen_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen_container, 0, 0);
    lv_obj_set_style_pad_all(screen_container, 0, 0);
    
    // === TOP: Time Display ===
    lv_obj_t *time_container = lv_obj_create(screen_container);
    lv_obj_set_size(time_container, 220, 100);
    lv_obj_align(time_container, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_bg_opa(time_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_container, 0, 0);
    lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(time_container, 8, 0);
    
    // Hour
    hour_label = lv_label_create(time_container);
    lv_label_set_text(hour_label, "12");
    lv_obj_set_style_text_font(hour_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(hour_label, COLOR_HOUR, 0);
    
    // Colon
    lv_obj_t *colon1 = lv_label_create(time_container);
    lv_label_set_text(colon1, ":");
    lv_obj_set_style_text_font(colon1, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(colon1, COLOR_MINUTE, 0);
    
    // Minute
    minute_label = lv_label_create(time_container);
    lv_label_set_text(minute_label, "30");
    lv_obj_set_style_text_font(minute_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(minute_label, COLOR_MINUTE, 0);
    
    // Colon
    lv_obj_t *colon2 = lv_label_create(time_container);
    lv_label_set_text(colon2, ":");
    lv_obj_set_style_text_font(colon2, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(colon2, COLOR_SECOND, 0);
    
    // Second
    second_label = lv_label_create(time_container);
    lv_label_set_text(second_label, "00");
    lv_obj_set_style_text_font(second_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(second_label, COLOR_SECOND, 0);
    
    // === Date and Weekday ===
    lv_obj_t *date_container = lv_obj_create(screen_container);
    lv_obj_set_size(date_container, 200, 50);
    lv_obj_align(date_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(date_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(date_container, 0, 0);
    lv_obj_set_flex_flow(date_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(date_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(date_container, 15, 0);
    
    // Weekday
    weekday_label = lv_label_create(date_container);
    lv_label_set_text(weekday_label, "Mon");
    lv_obj_set_style_text_font(weekday_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(weekday_label, COLOR_ACCENT, 0);
    
    // Separator dot
    lv_obj_t *dot = lv_label_create(date_container);
    lv_label_set_text(dot, "•");
    lv_obj_set_style_text_font(dot, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(dot, COLOR_DATE, 0);
    
    // Date
    date_label = lv_label_create(date_container);
    lv_label_set_text(date_label, "03/10");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(date_label, COLOR_DATE, 0);
    
    // === Battery Bar ===
    lv_obj_t *battery_container = lv_obj_create(screen_container);
    lv_obj_set_size(battery_container, 200, 40);
    lv_obj_align(battery_container, LV_ALIGN_CENTER, 0, 70);
    lv_obj_set_style_bg_opa(battery_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(battery_container, 0, 0);
    lv_obj_set_flex_flow(battery_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(battery_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(battery_container, 10, 0);
    
    // Battery icon (text)
    lv_obj_t *bat_icon = lv_label_create(battery_container);
    lv_label_set_text(bat_icon, "🔋");
    lv_obj_set_style_text_font(bat_icon, &lv_font_montserrat_24, 0);
    
    // Battery bar
    battery_bar = lv_bar_create(battery_container);
    lv_obj_set_size(battery_bar, 120, 18);
    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, 75, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(battery_bar, COLOR_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(battery_bar, COLOR_BATTERY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(battery_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    
    // Battery percentage
    battery_label = lv_label_create(battery_container);
    lv_label_set_text(battery_label, "75%");
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(battery_label, COLOR_BATTERY, 0);
    lv_obj_set_width(battery_label, 50);
    
    // === Bottom Status ===
    status_label = lv_label_create(screen_container);
    lv_label_set_text(status_label, "UP: 0s  H: 0KB");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label, COLOR_DATE, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Show on boot
    display_on = false;
    watch_face_user_activity();
    last_activity_time = start_time;
    
    ESP_LOGI(TAG, "Modern watch face initialized");
    
    // Start timer (1 second)
    lv_timer_create(timer_cb, 1000, NULL);
    
    // Initial update
    update_time();
    update_battery();
}
