/**
 * @file voice_recorder.c
 * @brief Voice Recorder and Player UI
 * 
 * Features:
 * - Swipe right to enter from watch face
 * - Record voice (using built-in microphone)
 * - Play back recordings
 * - List of recordings
 */

#include "voice_recorder.h"
#include "board_config.h"
#include "watch_face_chinese.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

// Forward declaration
extern void watch_face_chinese_show(void);

static const char *TAG = "VOICE_REC";

static lv_obj_t *recorder_screen = NULL;
static lv_obj_t *record_btn = NULL;
static lv_obj_t *play_btn = NULL;
static lv_obj_t *stop_btn = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *timer_label = NULL;
static lv_obj_t *list_container = NULL;

static bool is_recording = false;
static bool is_playing = false;
static uint32_t recording_start_time = 0;
static lv_timer_t *timer_update_timer = NULL;

// Colors
#define COLOR_BG        lv_color_hex(0x1a1a1a)
#define COLOR_RECORD    lv_color_hex(0xFF4444)
#define COLOR_PLAY      lv_color_hex(0x44AA44)
#define COLOR_STOP      lv_color_hex(0xFF8800)
#define COLOR_TEXT      lv_color_hex(0xFFFFFF)
#define COLOR_GRAY      lv_color_hex(0x888888)

// Timer update callback
static void timer_update_cb(lv_timer_t *timer) {
    (void)timer;
    
    if (is_recording) {
        uint32_t elapsed = (esp_timer_get_time() - recording_start_time) / 1000000;
        uint32_t mins = elapsed / 60;
        uint32_t secs = elapsed % 60;
        
        char buf[20];
        snprintf(buf, sizeof(buf), "%02lu:%02lu", mins, secs);
        lv_label_set_text(timer_label, buf);
    }
}

// Record button event callback
static void record_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        if (!is_recording) {
            // Start recording
            ESP_LOGI(TAG, "Start recording");
            is_recording = true;
            recording_start_time = esp_timer_get_time();
            
            lv_label_set_text(status_label, "Recording...");
            lv_obj_set_style_bg_color(record_btn, COLOR_GRAY, LV_PART_MAIN);
            lv_obj_set_style_bg_color(play_btn, COLOR_GRAY, LV_PART_MAIN);
            lv_obj_set_style_bg_color(stop_btn, COLOR_STOP, LV_PART_MAIN);
            
            // Start timer
            if (!timer_update_timer) {
                timer_update_timer = lv_timer_create(timer_update_cb, 1000, NULL);
            }
        } else {
            // Stop recording
            ESP_LOGI(TAG, "Stop recording");
            is_recording = false;
            
            lv_label_set_text(status_label, "Recording saved");
            lv_obj_set_style_bg_color(record_btn, COLOR_RECORD, LV_PART_MAIN);
            lv_obj_set_style_bg_color(play_btn, COLOR_PLAY, LV_PART_MAIN);
            lv_obj_set_style_bg_color(stop_btn, COLOR_GRAY, LV_PART_MAIN);
            
            // Add to list - SKIP for now (will be in separate screen)
            /*
            if (list_container) {
                lv_obj_t *item = lv_btn_create(list_container);
                lv_obj_set_size(item, lv_pct(100), 50);
                lv_obj_set_style_bg_color(item, lv_color_hex(0x333333), LV_PART_MAIN);
                
                lv_obj_t *label = lv_label_create(item);
                char buf[30];
                uint32_t elapsed = (esp_timer_get_time() - recording_start_time) / 1000000;
                snprintf(buf, sizeof(buf), "Rec %02lu:%02lu", elapsed / 60, elapsed % 60);
                lv_label_set_text(label, buf);
                lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
                lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
            }
            */
        }
    }
}

// Play button event callback
static void play_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED && !is_recording) {
        ESP_LOGI(TAG, "Play recording");
        lv_label_set_text(status_label, "Playing...");
        is_playing = true;
        
        // TODO: Implement actual playback
    }
}

// Stop button event callback
static void stop_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        if (is_recording) {
            // Stop recording
            is_recording = false;
            lv_label_set_text(status_label, "Recording saved");
            lv_obj_set_style_bg_color(record_btn, COLOR_RECORD, LV_PART_MAIN);
            lv_obj_set_style_bg_color(play_btn, COLOR_PLAY, LV_PART_MAIN);
            lv_obj_set_style_bg_color(stop_btn, COLOR_GRAY, LV_PART_MAIN);
        } else if (is_playing) {
            // Stop playback
            is_playing = false;
            lv_label_set_text(status_label, "Stopped");
        }
    }
}

// Back button event callback (swipe left to go back)
static void back_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Back to watch face");
        // Hide recorder screen and show watch face
        if (recorder_screen) {
            lv_obj_add_flag(recorder_screen, LV_OBJ_FLAG_HIDDEN);
        }
        watch_face_chinese_show();
    }
}

void voice_recorder_init(void) {
    ESP_LOGI(TAG, "Initializing voice recorder...");
    
    // Create screen
    recorder_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(recorder_screen, COLOR_BG, 0);
    
    // Header
    lv_obj_t *header = lv_obj_create(recorder_screen);
    lv_obj_set_size(header, lv_pct(100), 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(back_btn, COLOR_GRAY, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "←");
    lv_obj_set_style_text_color(back_label, COLOR_TEXT, 0);
    lv_obj_align(back_label, LV_ALIGN_CENTER, 0, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Voice Recorder");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    // Status label
    status_label = lv_label_create(recorder_screen);
    lv_label_set_text(status_label, "Ready");
    lv_obj_set_style_text_color(status_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 70);
    
    // Timer label
    timer_label = lv_label_create(recorder_screen);
    lv_label_set_text(timer_label, "00:00");
    lv_obj_set_style_text_color(timer_label, COLOR_RECORD, 0);
    lv_obj_set_style_text_font(timer_label, &lv_font_montserrat_48, 0);
    lv_obj_align(timer_label, LV_ALIGN_CENTER, 0, -30);
    
    // Control buttons container
    lv_obj_t *btn_container = lv_obj_create(recorder_screen);
    lv_obj_set_size(btn_container, lv_pct(100), 100);
    lv_obj_align(btn_container, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Record button
    record_btn = lv_btn_create(btn_container);
    lv_obj_set_size(record_btn, 70, 70);
    lv_obj_set_style_bg_color(record_btn, COLOR_RECORD, LV_PART_MAIN);
    lv_obj_set_style_radius(record_btn, 35, LV_PART_MAIN);
    lv_obj_add_event_cb(record_btn, record_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *record_icon = lv_label_create(record_btn);
    lv_label_set_text(record_icon, "●");
    lv_obj_set_style_text_color(record_icon, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(record_icon, &lv_font_montserrat_24, 0);
    lv_obj_align(record_icon, LV_ALIGN_CENTER, 0, 0);
    
    // Stop button
    stop_btn = lv_btn_create(btn_container);
    lv_obj_set_size(stop_btn, 70, 70);
    lv_obj_set_style_bg_color(stop_btn, COLOR_GRAY, LV_PART_MAIN);
    lv_obj_set_style_radius(stop_btn, 35, LV_PART_MAIN);
    lv_obj_add_event_cb(stop_btn, stop_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *stop_icon = lv_label_create(stop_btn);
    lv_label_set_text(stop_icon, "■");
    lv_obj_set_style_text_color(stop_icon, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(stop_icon, &lv_font_montserrat_24, 0);
    lv_obj_align(stop_icon, LV_ALIGN_CENTER, 0, 0);
    
    // Play button
    play_btn = lv_btn_create(btn_container);
    lv_obj_set_size(play_btn, 70, 70);
    lv_obj_set_style_bg_color(play_btn, COLOR_PLAY, LV_PART_MAIN);
    lv_obj_set_style_radius(play_btn, 35, LV_PART_MAIN);
    lv_obj_add_event_cb(play_btn, play_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *play_icon = lv_label_create(play_btn);
    lv_label_set_text(play_icon, "▶");
    lv_obj_set_style_text_color(play_icon, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(play_icon, &lv_font_montserrat_24, 0);
    lv_obj_align(play_icon, LV_ALIGN_CENTER, 0, 0);
    
    // Recording list container - HIDDEN (will be in separate screen)
    list_container = NULL;
    /*
    list_container = lv_obj_create(recorder_screen);
    lv_obj_set_size(list_container, lv_pct(90), 150);
    lv_obj_align(list_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_border_width(list_container, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(list_container, LV_DIR_VER);
    */
    
    ESP_LOGI(TAG, "Voice recorder initialized");
}

void voice_recorder_show(void) {
    if (recorder_screen) {
        lv_obj_clear_flag(recorder_screen, LV_OBJ_FLAG_HIDDEN);
        lv_scr_load(recorder_screen);
        ESP_LOGI(TAG, "Voice recorder screen shown");
    }
}

void voice_recorder_hide(void) {
    if (recorder_screen) {
        lv_obj_add_flag(recorder_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Voice recorder screen hidden");
    }
}
