/**
 * @file test_menu.c
 * @brief LVGL Test Menu - Component testing interface
 */

#include "test_menu.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "TEST_MENU";

// LVGL objects
static lv_obj_t *main_menu = NULL;
static lv_obj_t *current_screen = NULL;

// Test screen functions (to be implemented)
extern void test_display_create(lv_obj_t *parent);
extern void test_imu_create(lv_obj_t *parent);
extern void test_battery_create(lv_obj_t *parent);
extern void test_touch_create(lv_obj_t *parent);
extern void test_audio_create(lv_obj_t *parent);
extern void test_ble_create(lv_obj_t *parent);

// Menu items
typedef struct {
    const char *name;
    const char *icon;
    lv_event_cb_t callback;
} menu_item_t;

static void menu_item_click_cb(lv_event_t *e) {
    const char *name = (const char *)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Menu item clicked: %s", name);
    
    // Clear current screen
    if (current_screen) {
        lv_obj_clean(current_screen);
    }
    
    // Create test screen based on name
    if (strcmp(name, "Display") == 0) {
        test_display_create(current_screen);
    } else if (strcmp(name, "IMU") == 0) {
        test_imu_create(current_screen);
    } else if (strcmp(name, "Battery") == 0) {
        test_battery_create(current_screen);
    } else if (strcmp(name, "Touch") == 0) {
        test_touch_create(current_screen);
    } else if (strcmp(name, "Audio") == 0) {
        test_audio_create(current_screen);
    } else if (strcmp(name, "BLE") == 0) {
        test_ble_create(current_screen);
    }
}

static void back_button_cb(lv_event_t *e) {
    ESP_LOGI(TAG, "Back button clicked");
    // Return to main menu
    if (main_menu) {
        lv_scr_load(main_menu);
    }
}

void test_menu_init(void) {
    ESP_LOGI(TAG, "Initializing test menu...");
    
    // Create main menu screen
    main_menu = lv_obj_create(NULL);
    lv_obj_set_flex_flow(main_menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_menu, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(main_menu, lv_color_hex(0x1a1a1a), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(main_menu);
    lv_label_set_text(title, "ESP32-S3 Watch\nTest Menu");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(title, 20, 0);
    
    // Menu items
    menu_item_t items[] = {
        {"Display", "🖥️", menu_item_click_cb},
        {"Touch", "👆", menu_item_click_cb},
        {"IMU", "📟", menu_item_click_cb},
        {"Audio", "🔊", menu_item_click_cb},
        {"Battery", "🔋", menu_item_click_cb},
        {"BLE", "📶", menu_item_click_cb},
    };
    
    // Create menu buttons
    for (size_t i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        lv_obj_t *btn = lv_btn_create(main_menu);
        lv_obj_set_width(btn, 200);
        lv_obj_set_style_pad_top(btn, 5, 0);
        lv_obj_set_style_pad_bottom(btn, 5, 0);
        lv_obj_add_event_cb(btn, menu_item_click_cb, LV_EVENT_CLICKED, (void *)items[i].name);
        
        lv_obj_t *label = lv_label_create(btn);
        char text[32];
        snprintf(text, sizeof(text), "%s %s", items[i].icon, items[i].name);
        lv_label_set_text(label, text);
        lv_obj_center(label);
    }
    
    ESP_LOGI(TAG, "Test menu initialized with %d items", (int)(sizeof(items)/sizeof(items[0])));
}

void test_menu_show(void) {
    ESP_LOGI(TAG, "Showing test menu");
    if (main_menu) {
        lv_scr_load(main_menu);
    }
}

lv_obj_t* test_menu_create_test_screen(const char *title) {
    // Create new screen
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1a1a1a), 0);
    
    // Header with back button
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_add_event_cb(back_btn, back_button_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Title
    lv_obj_t *title_label = lv_label_create(header);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    
    // Content area
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_style_bg_color(content, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    
    current_screen = content;
    
    return content;
}
