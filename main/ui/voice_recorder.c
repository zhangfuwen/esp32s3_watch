/**
 * @file voice_recorder.c
 * @brief Voice Recorder and Player UI - Premium Design
 * 
 * Features:
 * - Swipe right to enter from watch face
 * - Record voice (using built-in microphone)
 * - Play back recordings
 * - Modern gradient UI with animations
 */

#include "voice_recorder.h"
#include "board_config.h"
#include "watch_face_chinese.h"
#include "i2s_audio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <string.h>

// Forward declaration
extern void watch_face_chinese_show(void);

static const char *TAG = "VOICE_REC";

// Recording buffer (5 seconds at 16kHz, 16-bit mono = 160KB)
#define RECORD_BUFFER_SIZE  (16000 * 2 * 5)  // 5 seconds
static int16_t *record_buffer = NULL;
static size_t recorded_bytes = 0;

// Static variables
static lv_obj_t *recorder_screen = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *timer_label = NULL;
static lv_obj_t *record_btn = NULL;
static lv_obj_t *stop_btn = NULL;
static lv_obj_t *play_btn = NULL;
static lv_obj_t *list_container = NULL;

static bool is_recording = false;
static bool is_playing = false;
static uint64_t recording_start_time = 0;
static lv_timer_t *timer = NULL;

// Colors - Modern gradient theme
#define COLOR_BG_TOP      lv_color_hex(0x1a1a2e)
#define COLOR_BG_BOTTOM   lv_color_hex(0x16213e)
#define COLOR_ACCENT      lv_color_hex(0xe94560)
#define COLOR_RECORD      lv_color_hex(0xff4757)
#define COLOR_PLAY        lv_color_hex(0x2ed573)
#define COLOR_STOP        lv_color_hex(0xffa502)
#define COLOR_TEXT        lv_color_hex(0xffffff)
#define COLOR_TEXT_DIM    lv_color_hex(0xa4b0be)
#define COLOR_CARD        lv_color_hex(0x0f3460)

// Timer callback
static void timer_cb(lv_timer_t *t) {
    if (is_recording) {
        uint32_t elapsed = (esp_timer_get_time() - recording_start_time) / 1000000;
        uint32_t minutes = elapsed / 60;
        uint32_t seconds = elapsed % 60;
        char buf[20];
        snprintf(buf, sizeof(buf), "%02lu:%02lu", minutes, seconds);
        lv_label_set_text(timer_label, buf);
        
        // Record audio data
        if (record_buffer && recorded_bytes < RECORD_BUFFER_SIZE) {
            int16_t temp_buf[256];
            esp_err_t ret = i2s_audio_record(temp_buf, sizeof(temp_buf));
            if (ret == ESP_OK) {
                size_t to_copy = sizeof(temp_buf);
                if (recorded_bytes + to_copy > RECORD_BUFFER_SIZE) {
                    to_copy = RECORD_BUFFER_SIZE - recorded_bytes;
                }
                memcpy((uint8_t*)record_buffer + recorded_bytes, temp_buf, to_copy);
                recorded_bytes += to_copy;
            }
        }
    }
}

// Back button event callback
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

// Record button event callback
static void record_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        if (!is_recording) {
            // Start recording
            ESP_LOGI(TAG, "Start recording");
            is_recording = true;
            is_playing = false;
            recording_start_time = esp_timer_get_time();
            recorded_bytes = 0;
            
            // Allocate buffer if not already
            if (!record_buffer) {
                record_buffer = heap_caps_malloc(RECORD_BUFFER_SIZE, MALLOC_CAP_INTERNAL);
                if (!record_buffer) {
                    record_buffer = heap_caps_malloc(RECORD_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
                }
            }
            
            lv_label_set_text(status_label, "Recording...");
            lv_obj_set_style_bg_color(record_btn, COLOR_RECORD, LV_PART_MAIN);
            lv_obj_set_style_bg_color(play_btn, COLOR_TEXT_DIM, LV_PART_MAIN);
            lv_obj_set_style_bg_color(stop_btn, COLOR_STOP, LV_PART_MAIN);
        } else {
            // Stop recording
            ESP_LOGI(TAG, "Stop recording");
            is_recording = false;
            
            lv_label_set_text(status_label, "Recording saved");
            lv_obj_set_style_bg_color(record_btn, COLOR_RECORD, LV_PART_MAIN);
            lv_obj_set_style_bg_color(play_btn, COLOR_PLAY, LV_PART_MAIN);
            lv_obj_set_style_bg_color(stop_btn, COLOR_TEXT_DIM, LV_PART_MAIN);
            
            // Reset timer
            lv_label_set_text(timer_label, "00:00");
        }
    }
}

// Play button event callback
static void play_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED && !is_recording) {
        ESP_LOGI(TAG, "Play recording");
        if (!is_playing) {
            if (recorded_bytes > 0) {
                // Play the recorded audio
                is_playing = true;
                lv_label_set_text(status_label, "Playing...");
                lv_obj_set_style_bg_color(play_btn, COLOR_PLAY, LV_PART_MAIN);
                
                // Simple playback (just play once)
                i2s_audio_play(record_buffer, recorded_bytes);
                
                is_playing = false;
                lv_label_set_text(status_label, "Ready to Record");
                lv_obj_set_style_bg_color(play_btn, COLOR_TEXT_DIM, LV_PART_MAIN);
            } else {
                lv_label_set_text(status_label, "No recording to play");
            }
        } else {
            is_playing = false;
            lv_label_set_text(status_label, "Ready to Record");
            lv_obj_set_style_bg_color(play_btn, COLOR_TEXT_DIM, LV_PART_MAIN);
        }
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
            lv_obj_set_style_bg_color(stop_btn, COLOR_TEXT_DIM, LV_PART_MAIN);
            lv_label_set_text(timer_label, "00:00");
        } else if (is_playing) {
            // Stop playback
            is_playing = false;
            lv_label_set_text(status_label, "Ready to Record");
            lv_obj_set_style_bg_color(play_btn, COLOR_TEXT_DIM, LV_PART_MAIN);
        }
    }
}

void voice_recorder_init(void) {
    ESP_LOGI(TAG, "Initializing voice recorder (Premium UI)...");
    
    // Initialize I2S audio
    i2s_audio_init();
    
    // Create screen with gradient background
    recorder_screen = lv_obj_create(NULL);
    
    // Create gradient background
    lv_obj_set_style_bg_color(recorder_screen, COLOR_BG_TOP, 0);
    lv_obj_set_style_bg_grad_dir(recorder_screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_color(recorder_screen, COLOR_BG_BOTTOM, 0);
    
    // Header with subtle background
    lv_obj_t *header = lv_obj_create(recorder_screen);
    lv_obj_set_size(header, lv_pct(100), 60);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
    
    // Back button with modern style
    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_bg_color(back_btn, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 25, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(back_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(back_btn, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Back icon
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
    lv_obj_align(back_label, LV_ALIGN_CENTER, 0, 0);
    
    // Title with icon
    lv_obj_t *title_icon = lv_label_create(header);
    lv_label_set_text(title_icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(title_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(title_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(title_icon, LV_ALIGN_CENTER, -40, 0);
    
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Voice Recorder");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 20, 0);
    
    // Status label with larger font
    status_label = lv_label_create(recorder_screen);
    lv_label_set_text(status_label, "Ready to Record");
    lv_obj_set_style_text_color(status_label, COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 75);
    
    // Timer label - large and prominent
    timer_label = lv_label_create(recorder_screen);
    lv_label_set_text(timer_label, "00:00");
    lv_obj_set_style_text_color(timer_label, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(timer_label, &lv_font_montserrat_48, 0);
    lv_obj_align(timer_label, LV_ALIGN_CENTER, 0, -20);
    
    // Control buttons container with card background
    lv_obj_t *btn_card = lv_obj_create(recorder_screen);
    lv_obj_set_size(btn_card, lv_pct(95), 110);
    lv_obj_align(btn_card, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_card, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn_card, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_card, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_card, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_card, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Record button with glow effect
    record_btn = lv_btn_create(btn_card);
    lv_obj_set_size(record_btn, 75, 75);
    lv_obj_set_style_bg_color(record_btn, COLOR_RECORD, LV_PART_MAIN);
    lv_obj_set_style_radius(record_btn, 38, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(record_btn, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(record_btn, COLOR_RECORD, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(record_btn, LV_OPA_50, LV_PART_MAIN);
    lv_obj_add_event_cb(record_btn, record_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *record_icon = lv_label_create(record_btn);
    lv_label_set_text(record_icon, "●");
    lv_obj_set_style_text_color(record_icon, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(record_icon, &lv_font_montserrat_32, 0);
    lv_obj_align(record_icon, LV_ALIGN_CENTER, 0, 0);
    
    // Stop button
    stop_btn = lv_btn_create(btn_card);
    lv_obj_set_size(stop_btn, 75, 75);
    lv_obj_set_style_bg_color(stop_btn, COLOR_STOP, LV_PART_MAIN);
    lv_obj_set_style_radius(stop_btn, 38, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(stop_btn, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(stop_btn, COLOR_STOP, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(stop_btn, LV_OPA_50, LV_PART_MAIN);
    lv_obj_add_event_cb(stop_btn, stop_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *stop_icon = lv_label_create(stop_btn);
    lv_label_set_text(stop_icon, "■");
    lv_obj_set_style_text_color(stop_icon, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(stop_icon, &lv_font_montserrat_32, 0);
    lv_obj_align(stop_icon, LV_ALIGN_CENTER, 0, 0);
    
    // Play button
    play_btn = lv_btn_create(btn_card);
    lv_obj_set_size(play_btn, 75, 75);
    lv_obj_set_style_bg_color(play_btn, COLOR_PLAY, LV_PART_MAIN);
    lv_obj_set_style_radius(play_btn, 38, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(play_btn, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(play_btn, COLOR_PLAY, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(play_btn, LV_OPA_50, LV_PART_MAIN);
    lv_obj_add_event_cb(play_btn, play_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *play_icon = lv_label_create(play_btn);
    lv_label_set_text(play_icon, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(play_icon, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(play_icon, &lv_font_montserrat_32, 0);
    lv_obj_align(play_icon, LV_ALIGN_CENTER, 0, 0);
    
    // Recording list - HIDDEN (will be in separate screen)
    list_container = NULL;
    
    // Create timer for recording
    timer = lv_timer_create(timer_cb, 100, NULL);
    
    ESP_LOGI(TAG, "Voice recorder initialized (Premium UI)");
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
    }
}
