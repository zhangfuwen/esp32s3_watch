/**
 * @file test_display.c
 * @brief LVGL Display Test - Color and screen test
 */

#include "test_menu.h"
#include "esp_log.h"
#include <lvgl/lvgl.h>

static const char *TAG = "TEST_DISPLAY";

static lv_obj_t *color_blocks[8];
static const uint32_t test_colors[8] = {
    0xFF0000,  // Red
    0x00FF00,  // Green
    0x0000FF,  // Blue
    0xFFFF00,  // Yellow
    0xFF00FF,  // Magenta
    0x00FFFF,  // Cyan
    0xFFFFFF,  // White
    0x000000,  // Black
};

static const char *color_names[8] = {
    "RED", "GREEN", "BLUE", "YELLOW", "MAGENTA", "CYAN", "WHITE", "BLACK"
};

static void update_color_blocks(lv_obj_t *parent) {
    for (int i = 0; i < 8; i++) {
        if (color_blocks[i]) {
            lv_obj_clean(color_blocks[i]);
        }
        
        color_blocks[i] = lv_obj_create(parent);
        lv_obj_set_size(color_blocks[i], LV_PCT(50), 30);
        lv_obj_set_style_bg_color(color_blocks[i], lv_color_hex(test_colors[i]), 0);
        lv_obj_set_style_border_width(color_blocks[i], 1, 0);
        lv_obj_set_style_border_color(color_blocks[i], lv_color_hex(0xFFFFFF), 0);
        
        lv_obj_t *label = lv_label_create(color_blocks[i]);
        lv_label_set_text(label, color_names[i]);
        lv_obj_set_style_text_color(label, 
            (i == 7) ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0);
        lv_obj_center(label);
    }
}

void test_display_create(lv_obj_t *parent) {
    ESP_LOGI(TAG, "Display test started");
    
    lv_obj_t *screen = test_menu_create_test_screen("Display Test");
    
    lv_obj_t *info_label = lv_label_create(screen);
    lv_label_set_text(info_label, "Color Test Pattern");
    lv_obj_set_style_text_color(info_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_margin_bottom(info_label, 10, 0);
    
    update_color_blocks(screen);
    
    lv_obj_t *desc_label = lv_label_create(screen);
    lv_label_set_text(desc_label, "Screen: 240x285\nColors: 262K");
    lv_obj_set_style_text_color(desc_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_margin_top(desc_label, 10, 0);
}
