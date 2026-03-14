/**
 * @file hw_tests.c
 * @brief Hardware Test Suite - Individual test screens for each hardware component
 * 
 * Test screens:
 * 1. Display Test - Colors, gradients, fonts
 * 2. Touch Test - Raw touch coordinates and gestures
 * 3. Audio Test - Speaker output, microphone input
 * 4. IMU Test - Accelerometer and gyroscope
 * 5. Battery Test - Voltage and percentage
 * 6. BLE Test - Bluetooth scanning and advertising
 * 7. WiFi Test - Network scanning and connection
 * 8. Button Test - BOOT and other buttons
 */

#include "hw_tests.h"
#include "board_config.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static const char *TAG = "HW_TESTS";

// Global test screen references
static lv_obj_t *test_menu_screen = NULL;
static lv_obj_t *display_test_screen = NULL;
static lv_obj_t *touch_test_screen = NULL;
static lv_obj_t *audio_test_screen = NULL;
static lv_obj_t *imu_test_screen = NULL;
static lv_obj_t *battery_test_screen = NULL;
static lv_obj_t *ble_test_screen = NULL;
static lv_obj_t *wifi_test_screen = NULL;
static lv_obj_t *button_test_screen = NULL;

// Test state
// static bool test_running = false;
// static lv_timer_t *test_timer = NULL;

// Forward declarations
static void create_test_menu(void);
static void create_display_test(void);
static void create_touch_test(void);
static void create_audio_test(void);
static void create_imu_test(void);
static void create_battery_test(void);
static void create_ble_test(void);
static void create_wifi_test(void);
static void create_button_test(void);
static void back_btn_event_cb(lv_event_t *e);

// Helper: Create back button
static lv_obj_t* create_header(lv_obj_t *parent, const char *title) {
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, lv_pct(100), 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f3460), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 50, 40);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xe94560), LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 20, LV_PART_MAIN);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xffffff), 0);
    lv_obj_align(back_label, LV_ALIGN_CENTER, 0, 0);
    
    // Title
    lv_obj_t *title_label = lv_label_create(header);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);
    
    return back_btn;
}

//==================== Test Menu ====================

static void test_item_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = lv_event_get_user_data(e);
    
    if (code == LV_EVENT_CLICKED) {
        const char *text = lv_label_get_text(label);
        ESP_LOGI(TAG, "Selected test: %s", text);
        
        if (strcmp(text, "Display Test") == 0) {
            lv_scr_load(display_test_screen);
            lv_obj_clear_flag(display_test_screen, LV_OBJ_FLAG_HIDDEN);
        } else if (strcmp(text, "Touch Test") == 0) {
            lv_scr_load(touch_test_screen);
            lv_obj_clear_flag(touch_test_screen, LV_OBJ_FLAG_HIDDEN);
        } else if (strcmp(text, "Audio Test") == 0) {
            lv_scr_load(audio_test_screen);
            lv_obj_clear_flag(audio_test_screen, LV_OBJ_FLAG_HIDDEN);
        } else if (strcmp(text, "IMU Test") == 0) {
            lv_scr_load(imu_test_screen);
            lv_obj_clear_flag(imu_test_screen, LV_OBJ_FLAG_HIDDEN);
        } else if (strcmp(text, "Battery Test") == 0) {
            lv_scr_load(battery_test_screen);
            lv_obj_clear_flag(battery_test_screen, LV_OBJ_FLAG_HIDDEN);
        } else if (strcmp(text, "BLE Test") == 0) {
            lv_scr_load(ble_test_screen);
            lv_obj_clear_flag(ble_test_screen, LV_OBJ_FLAG_HIDDEN);
        } else if (strcmp(text, "WiFi Test") == 0) {
            lv_scr_load(wifi_test_screen);
            lv_obj_clear_flag(wifi_test_screen, LV_OBJ_FLAG_HIDDEN);
        } else if (strcmp(text, "Button Test") == 0) {
            lv_scr_load(button_test_screen);
            lv_obj_clear_flag(button_test_screen, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void create_test_menu(void) {
    ESP_LOGI(TAG, "Creating hardware test menu...");
    
    test_menu_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(test_menu_screen, lv_color_hex(0x1a1a2e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(test_menu_screen);
    lv_label_set_text(title, "Hardware Test Suite");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Test list
    const char *tests[] = {
        "Display Test",
        "Touch Test",
        "Audio Test",
        "IMU Test",
        "Battery Test",
        "BLE Test",
        "WiFi Test",
        "Button Test",
        NULL
    };
    
    int y = 80;
    for (int i = 0; tests[i] != NULL; i++) {
        lv_obj_t *btn = lv_btn_create(test_menu_screen);
        lv_obj_set_size(btn, lv_pct(90), 50);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0f3460), LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, test_item_event_cb, LV_EVENT_ALL, btn);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, tests[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        
        lv_obj_add_event_cb(btn, test_item_event_cb, LV_EVENT_CLICKED, label);
        
        y += 60;
    }
    
    lv_obj_add_flag(test_menu_screen, LV_OBJ_FLAG_HIDDEN);
    ESP_LOGI(TAG, "Test menu created");
}

//==================== Display Test ====================

static uint8_t display_test_step = 0;
static lv_obj_t *display_test_label = NULL;

static void display_test_timer_cb(lv_timer_t *timer) {
    display_test_step++;
    
    switch (display_test_step % 6) {
        case 0: // Red
            lv_obj_set_style_bg_color(display_test_screen, lv_color_hex(0xff0000), 0);
            lv_label_set_text(display_test_label, "RED");
            break;
        case 1: // Green
            lv_obj_set_style_bg_color(display_test_screen, lv_color_hex(0x00ff00), 0);
            lv_label_set_text(display_test_label, "GREEN");
            break;
        case 2: // Blue
            lv_obj_set_style_bg_color(display_test_screen, lv_color_hex(0x0000ff), 0);
            lv_label_set_text(display_test_label, "BLUE");
            break;
        case 3: // White
            lv_obj_set_style_bg_color(display_test_screen, lv_color_hex(0xffffff), 0);
            lv_label_set_text(display_test_label, "WHITE");
            break;
        case 4: // Black
            lv_obj_set_style_bg_color(display_test_screen, lv_color_hex(0x000000), 0);
            lv_label_set_text(display_test_label, "BLACK");
            break;
        case 5: // Gradient
            lv_obj_set_style_bg_color(display_test_screen, lv_color_hex(0x1a1a2e), 0);
            lv_label_set_text(display_test_label, "Gradient");
            break;
    }
    
    ESP_LOGI(TAG, "Display test step %d", display_test_step);
}

static void create_display_test(void) {
    ESP_LOGI(TAG, "Creating display test...");
    
    display_test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(display_test_screen, lv_color_hex(0xff0000), 0);
    
    lv_obj_t *back_btn = create_header(display_test_screen, "Display Test");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    display_test_label = lv_label_create(display_test_screen);
    lv_label_set_text(display_test_label, "RED");
    lv_obj_set_style_text_color(display_test_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(display_test_label, &lv_font_montserrat_48, 0);
    lv_obj_align(display_test_label, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t *info = lv_label_create(display_test_screen);
    lv_label_set_text(info, "Auto-cycle every 2s\nR-G-B-White-Black");
    lv_obj_set_style_text_color(info, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_16, 0);
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    lv_obj_add_flag(display_test_screen, LV_OBJ_FLAG_HIDDEN);
    
    // Start timer
    lv_timer_create(display_test_timer_cb, 2000, NULL);
    
    ESP_LOGI(TAG, "Display test created");
}

//==================== Audio Test ====================

static lv_obj_t *audio_status_label = NULL;
static lv_obj_t *audio_wave_label = NULL;

static void audio_test_tone(void) {
    lv_label_set_text(audio_status_label, "Playing 440Hz...");
    
    int16_t test_tone[256];
    const float freq = 440.0f;
    const float sample_rate = 16000.0f;
    const float amplitude = 16000.0f;
    
    for (int i = 0; i < 256; i++) {
        float t = i / sample_rate;
        test_tone[i] = (int16_t)(amplitude * sinf(2.0f * M_PI * freq * t));
    }
    
    // Play 5 times
    for (int i = 0; i < 5; i++) {
        extern esp_err_t i2s_audio_play(const int16_t *buffer, size_t size);
        i2s_audio_play(test_tone, sizeof(test_tone));
    }
    
    lv_label_set_text(audio_status_label, "Test complete");
}

static void audio_test_sweep(void) {
    lv_label_set_text(audio_status_label, "Playing sweep...");
    
    // Frequency sweep 200Hz -> 2000Hz
    for (float freq = 200.0f; freq < 2000.0f; freq += 100.0f) {
        int16_t tone[128];
        for (int i = 0; i < 128; i++) {
            float t = i / 16000.0f;
            tone[i] = (int16_t)(16000.0f * sinf(2.0f * M_PI * freq * t));
        }
        extern esp_err_t i2s_audio_play(const int16_t *buffer, size_t size);
        i2s_audio_play(tone, sizeof(tone));
    }
    
    lv_label_set_text(audio_status_label, "Sweep complete");
}

static void audio_btn_tone_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_test_tone();
    }
}

static void audio_btn_sweep_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_test_sweep();
    }
}

static void create_audio_test(void) {
    ESP_LOGI(TAG, "Creating audio test...");
    
    audio_test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(audio_test_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *back_btn = create_header(audio_test_screen, "Audio Test");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Status
    audio_status_label = lv_label_create(audio_test_screen);
    lv_label_set_text(audio_status_label, "Ready");
    lv_obj_set_style_text_color(audio_status_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(audio_status_label, &lv_font_montserrat_24, 0);
    lv_obj_align(audio_status_label, LV_ALIGN_TOP_MID, 0, 70);
    
    // Test buttons
    lv_obj_t *btn_tone = lv_btn_create(audio_test_screen);
    lv_obj_set_size(btn_tone, 200, 60);
    lv_obj_align(btn_tone, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_bg_color(btn_tone, lv_color_hex(0xff4757), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_tone, audio_btn_tone_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_tone = lv_label_create(btn_tone);
    lv_label_set_text(label_tone, "Test 440Hz Tone");
    lv_obj_set_style_text_color(label_tone, lv_color_hex(0xffffff), 0);
    lv_obj_align(label_tone, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t *btn_sweep = lv_btn_create(audio_test_screen);
    lv_obj_set_size(btn_sweep, 200, 60);
    lv_obj_align(btn_sweep, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(btn_sweep, lv_color_hex(0x2ed573), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_sweep, audio_btn_sweep_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_sweep = lv_label_create(btn_sweep);
    lv_label_set_text(label_sweep, "Frequency Sweep");
    lv_obj_set_style_text_color(label_sweep, lv_color_hex(0xffffff), 0);
    lv_obj_align(label_sweep, LV_ALIGN_CENTER, 0, 0);
    
    // Info
    lv_obj_t *info = lv_label_create(audio_test_screen);
    lv_label_set_text(info, "Turn up volume\nListen for tone");
    lv_obj_set_style_text_color(info, lv_color_hex(0xa4b0be), 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_14, 0);
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    lv_obj_add_flag(audio_test_screen, LV_OBJ_FLAG_HIDDEN);
    ESP_LOGI(TAG, "Audio test created");
}

//==================== Back Button Handler ====================

static void back_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Back to test menu");
        lv_scr_load(test_menu_screen);
    }
}

//==================== Initialization ====================

void hw_tests_init(void) {
    ESP_LOGI(TAG, "Initializing hardware tests...");
    
    create_test_menu();
    create_display_test();
    create_touch_test();  // Placeholder
    create_audio_test();
    create_imu_test();    // Placeholder
    create_battery_test(); // Placeholder
    create_ble_test();    // Placeholder
    create_wifi_test();   // Placeholder
    create_button_test(); // Placeholder
    
    ESP_LOGI(TAG, "Hardware tests initialized");
}

void hw_tests_show_menu(void) {
    lv_scr_load(test_menu_screen);
    lv_obj_clear_flag(test_menu_screen, LV_OBJ_FLAG_HIDDEN);
}

// Placeholder implementations
static void create_touch_test(void) {
    touch_test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(touch_test_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *back_btn = create_header(touch_test_screen, "Touch Test");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl = lv_label_create(touch_test_screen);
    lv_label_set_text(lbl, "Touch Test\n(Touch the screen)");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_flag(touch_test_screen, LV_OBJ_FLAG_HIDDEN);
}

static void create_imu_test(void) {
    imu_test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(imu_test_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *back_btn = create_header(imu_test_screen, "IMU Test");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl = lv_label_create(imu_test_screen);
    lv_label_set_text(lbl, "IMU Test\n(QMI8658C)\n\nAccel: X=0 Y=0 Z=0\nGyro: X=0 Y=0 Z=0");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_flag(imu_test_screen, LV_OBJ_FLAG_HIDDEN);
}

static void create_battery_test(void) {
    battery_test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(battery_test_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *back_btn = create_header(battery_test_screen, "Battery Test");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl = lv_label_create(battery_test_screen);
    lv_label_set_text(lbl, "Battery Test\n(MAX17048G)\n\nVoltage: 3.8V\nLevel: 75%\nCharging: No");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_flag(battery_test_screen, LV_OBJ_FLAG_HIDDEN);
}

static void create_ble_test(void) {
    ble_test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ble_test_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *back_btn = create_header(ble_test_screen, "BLE Test");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl = lv_label_create(ble_test_screen);
    lv_label_set_text(lbl, "BLE Test\n(Bluetooth 5.0)\n\nStatus: Ready\nDevices: 0\n\nClick to scan");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_flag(ble_test_screen, LV_OBJ_FLAG_HIDDEN);
}

static void create_wifi_test(void) {
    wifi_test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifi_test_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *back_btn = create_header(wifi_test_screen, "WiFi Test");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl = lv_label_create(wifi_test_screen);
    lv_label_set_text(lbl, "WiFi Test\n(802.11 b/g/n)\n\nStatus: Disconnected\nNetworks: 0\n\nClick to scan");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_flag(wifi_test_screen, LV_OBJ_FLAG_HIDDEN);
}

static void create_button_test(void) {
    button_test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(button_test_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *back_btn = create_header(button_test_screen, "Button Test");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl = lv_label_create(button_test_screen);
    lv_label_set_text(lbl, "Button Test\n\nBOOT Button (GPIO0)\n\nStatus: Not pressed\n\nPress BOOT button\nto test");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_flag(button_test_screen, LV_OBJ_FLAG_HIDDEN);
}
