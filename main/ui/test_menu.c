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

// Test screen functions
extern void test_display_create(lv_obj_t *parent);
extern void test_imu_create(lv_obj_t *parent);
extern void test_battery_create(lv_obj_t *parent);
// TODO: extern void test_touch_create(lv_obj_t *parent);
// TODO: extern void test_audio_create(lv_obj_t *parent);
// TODO: extern void test_ble_create(lv_obj_t *parent);

// Menu items
typedef struct {
    const char *name;
    const char *icon;
    lv_event_cb_t callback;
} menu_item_t;

static void menu_item_click_cb(lv_event_t *e) {
    const char *name = (const char *)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Menu item clicked: %s", name);
    
    if (name == NULL) {
        ESP_LOGE(TAG, "NULL menu item name");
        return;
    }
    
    if (current_screen == NULL) {
        ESP_LOGE(TAG, "current_screen is NULL, cannot create test screen");
        return;
    }
    
    // Clear current screen
    lv_obj_clean(current_screen);
    
    // Create test screen based on name
    if (strcmp(name, "Display") == 0) {
        test_display_create(current_screen);
    } else if (strcmp(name, "IMU") == 0) {
        test_imu_create(current_screen);
    } else if (strcmp(name, "Battery") == 0) {
        test_battery_create(current_screen);
    } else {
        ESP_LOGW(TAG, "Unknown test: %s", name);
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
    
    // Check if LVGL is initialized
    if (!lv_is_initialized()) {
        ESP_LOGE(TAG, "LVGL not initialized! Call lv_init() first!");
        return;
    }
    ESP_LOGI(TAG, "LVGL is initialized");
    
    // Check if display driver is registered
    lv_disp_t *disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "No LVGL display driver registered! lv_disp_get_default() returned NULL");
        ESP_LOGE(TAG, "test_menu requires LVGL display driver to be set up first");
        return;
    }
    ESP_LOGI(TAG, "LVGL display driver: %p", disp);
    
    if (main_menu != NULL) {
        ESP_LOGW(TAG, "Test menu already initialized");
        return;
    }
    
    ESP_LOGI(TAG, "Creating main menu screen...");
    main_menu = lv_obj_create(NULL);
    if (main_menu == NULL) {
        ESP_LOGE(TAG, "Failed to create main_menu object");
        return;
    }
    lv_obj_set_flex_flow(main_menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_menu, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(main_menu, lv_color_hex(0x1a1a1a), 0);
    
    ESP_LOGI(TAG, "Creating title label...");
    lv_obj_t *title = lv_label_create(main_menu);
    if (title == NULL) {
        ESP_LOGE(TAG, "Failed to create title label");
        return;
    }
    lv_label_set_text(title, "ESP32-S3 Watch\nTest Menu");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(title, 20, 0);
    
    ESP_LOGI(TAG, "Creating menu items...");
    menu_item_t items[] = {
        {"Display", "D", menu_item_click_cb},
        {"IMU", "I", menu_item_click_cb},
        {"Battery", "B", menu_item_click_cb},
    };
    
    int num_items = sizeof(items)/sizeof(items[0]);
    ESP_LOGI(TAG, "Creating %d menu buttons...", num_items);
    
    for (int i = 0; i < num_items; i++) {
        if (items[i].callback == NULL) {
            ESP_LOGW(TAG, "Skipping menu item %s - NULL callback", items[i].name);
            continue;
        }
        
        lv_obj_t *btn = lv_btn_create(main_menu);
        if (btn == NULL) {
            ESP_LOGE(TAG, "Failed to create button for %s", items[i].name);
            continue;
        }
        lv_obj_set_width(btn, 200);
        lv_obj_set_style_pad_top(btn, 5, 0);
        lv_obj_set_style_pad_bottom(btn, 5, 0);
        lv_obj_add_event_cb(btn, items[i].callback, LV_EVENT_CLICKED, (void *)items[i].name);
        
        lv_obj_t *label = lv_label_create(btn);
        if (label == NULL) {
            ESP_LOGE(TAG, "Failed to create label for %s", items[i].name);
            continue;
        }
        char text[32];
        snprintf(text, sizeof(text), "[%s] %s", items[i].icon, items[i].name);
        lv_label_set_text(label, text);
        lv_obj_center(label);
        
        ESP_LOGI(TAG, "Created menu item: %s", items[i].name);
    }
    
    ESP_LOGI(TAG, "Test menu initialized with %d items", num_items);
}

void test_menu_show(void) {
    ESP_LOGI(TAG, "Showing test menu");
    if (main_menu) {
        lv_scr_load(main_menu);
    }
}

lv_obj_t* test_menu_create_test_screen(const char *test_name) {
    ESP_LOGI(TAG, "Creating test screen: %s", test_name ? test_name : "NULL");
    
    if (test_name == NULL) {
        ESP_LOGE(TAG, "test_name is NULL");
        return NULL;
    }
    
    if (main_menu == NULL) {
        ESP_LOGE(TAG, "main_menu is NULL, call test_menu_init first");
        return NULL;
    }
    
    lv_obj_t *screen = lv_obj_create(NULL);
    if (screen == NULL) {
        ESP_LOGE(TAG, "Failed to create screen");
        return NULL;
    }
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1a1a1a), 0);
    
    lv_obj_t *header = lv_obj_create(screen);
    if (header == NULL) {
        ESP_LOGE(TAG, "Failed to create header");
        lv_obj_del(screen);
        return NULL;
    }
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    
    lv_obj_t *back_btn = lv_btn_create(header);
    if (back_btn == NULL) {
        ESP_LOGE(TAG, "Failed to create back button");
        lv_obj_del(screen);
        return NULL;
    }
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_add_event_cb(back_btn, back_button_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    if (back_label) {
        lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
        lv_obj_center(back_label);
    }
    
    lv_obj_t *title_label = lv_label_create(header);
    if (title_label == NULL) {
        ESP_LOGE(TAG, "Failed to create title label");
        lv_obj_del(screen);
        return NULL;
    }
    lv_label_set_text(title_label, test_name);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_t *content = lv_obj_create(screen);
    if (content == NULL) {
        ESP_LOGE(TAG, "Failed to create content area");
        lv_obj_del(screen);
        return NULL;
    }
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_style_bg_color(content, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    
    current_screen = content;
    
    ESP_LOGI(TAG, "Test screen created: %s", test_name);
    return content;
}
