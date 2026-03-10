/**
 * @file lvgl_test.c
 * @brief Simple LVGL Test - Following working reference code
 */

#include "lvgl_test.h"
#include "board_config.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "LVGL_TEST";

static lv_obj_t *test_label = NULL;

esp_err_t lvgl_test_init(void) {
    ESP_LOGI(TAG, "LVGL Test Init");
    return ESP_OK;
}

esp_err_t lvgl_test_run(void) {
    ESP_LOGI(TAG, "=== LVGL Test Starting ===");
    
    // Create a simple test screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000080), 0);  // Dark blue background
    lv_scr_load(scr);
    
    // Create title label
    test_label = lv_label_create(scr);
    lv_label_set_text(test_label, "LVGL Test\n240x284");
    lv_obj_set_style_text_color(test_label, lv_color_white(), 0);
    lv_obj_align(test_label, LV_ALIGN_CENTER, 0, 0);
    
    // Create a button
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_size(btn, 120, 40);
    
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Working!");
    lv_obj_center(btn_label);
    
    // Create a bar (progress bar)
    lv_obj_t *bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 200, 20);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 60);
    lv_bar_set_value(bar, 75, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    
    ESP_LOGI(TAG, "LVGL test screen created");
    ESP_LOGI(TAG, "Screen size: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    return ESP_OK;
}
