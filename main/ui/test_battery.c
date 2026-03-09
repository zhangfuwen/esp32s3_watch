/**
 * @file test_battery.c
 * @brief LVGL Battery Test - MAX17048G fuel gauge display
 */

#include "test_menu.h"
#include "power_manager.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "TEST_BATTERY";

static lv_obj_t *battery_percent_label;
static lv_obj_t *battery_bar;
static lv_obj_t *charging_label;
static lv_obj_t *voltage_label;
static lv_obj_t *status_label;

void test_battery_create(lv_obj_t *parent) {
    ESP_LOGI(TAG, "Battery test started");
    
    lv_obj_t *screen = test_menu_create_test_screen("Battery Test");
    
    battery_percent_label = lv_label_create(screen);
    lv_label_set_text(battery_percent_label, "---%");
    lv_obj_set_style_text_color(battery_percent_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(battery_percent_label, &lv_font_montserrat_48, 0);
    
    battery_bar = lv_bar_create(screen);
    lv_obj_set_size(battery_bar, 200, 30);
    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x333333), LV_PART_INDICATOR);
    
    charging_label = lv_label_create(screen);
    lv_label_set_text(charging_label, "Charging: --");
    lv_obj_set_style_text_color(charging_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_pad_top(charging_label, 10, 0);
    
    voltage_label = lv_label_create(screen);
    lv_label_set_text(voltage_label, "Voltage: --.-- V");
    lv_obj_set_style_text_color(voltage_label, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_pad_top(voltage_label, 5, 0);
    
    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Status: Initializing...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_pad_top(status_label, 10, 0);
    
    test_battery_update();
}

void test_battery_update(void) {
    uint8_t level = power_manager_get_battery_level();
    bool charging = power_manager_is_charging();
    
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", level);
    lv_label_set_text(battery_percent_label, buf);
    lv_bar_set_value(battery_bar, level, LV_ANIM_OFF);
    
    if (level > 60) {
        lv_obj_set_style_text_color(battery_percent_label, lv_color_hex(0x00FF00), 0);
    } else if (level > 20) {
        lv_obj_set_style_text_color(battery_percent_label, lv_color_hex(0xFFFF00), 0);
    } else {
        lv_obj_set_style_text_color(battery_percent_label, lv_color_hex(0xFF0000), 0);
    }
    
    lv_label_set_text(charging_label, charging ? "Charging: Yes" : "Charging: No");
    
    float voltage = 3.7f + (level / 100.0f) * 0.5f;
    snprintf(buf, sizeof(buf), "Voltage: %.2f V", voltage);
    lv_label_set_text(voltage_label, buf);
    
    if (level > 80) {
        lv_label_set_text(status_label, "Status: Excellent");
    } else if (level > 50) {
        lv_label_set_text(status_label, "Status: Good");
    } else if (level > 20) {
        lv_label_set_text(status_label, "Status: Fair");
    } else {
        lv_label_set_text(status_label, "Status: Low!");
    }
    
    ESP_LOGI(TAG, "Battery: %d%%, Charging: %s", level, charging ? "Yes" : "No");
}
