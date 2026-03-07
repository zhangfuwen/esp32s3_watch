/**
 * @file messages.c
 * @brief Comfort Messages Implementation
 * 
 * Displays encouraging and supportive messages to provide
 * emotional comfort throughout the day.
 */

#include "messages.h"
#include "display.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

// ESP-IDF includes
#include "esp_log.h"
#include "esp_timer.h"

// Tag for logging
static const char* TAG = "messages";

// Default configuration
#define DEFAULT_MIN_INTERVAL_MINUTES  30
#define DEFAULT_MAX_DAILY_MESSAGES    10

// Message display duration (ms)
#define MESSAGE_DISPLAY_DURATION_MS  3000

// Built-in comfort messages (stored in flash)
static const comfort_message_t g_builtin_messages[] = {
    // Morning messages (6AM-12PM)
    {.text = "Good morning! Today is a fresh start.", .category = MSG_CATEGORY_MORNING},
    {.text = "You have everything you need within you.", .category = MSG_CATEGORY_MORNING},
    {.text = "One step at a time. You've got this.", .category = MSG_CATEGORY_MORNING},
    {.text = "Morning light, new possibilities.", .category = MSG_CATEGORY_MORNING},
    {.text = "Today holds opportunities. Be open to them.", .category = MSG_CATEGORY_MORNING},
    
    // Midday messages (12PM-6PM)
    {.text = "Take a breath. You're doing great.", .category = MSG_CATEGORY_MIDDAY},
    {.text = "Pause. Breathe. Continue.", .category = MSG_CATEGORY_MIDDAY},
    {.text = "Progress, not perfection.", .category = MSG_CATEGORY_MIDDAY},
    {.text = "You've made it through 100% of your hard days.", .category = MSG_CATEGORY_MIDDAY},
    {.text = "Midday reminder: You are capable.", .category = MSG_CATEGORY_MIDDAY},
    
    // Evening messages (6PM-12AM)
    {.text = "You did enough today.", .category = MSG_CATEGORY_EVENING},
    {.text = "Rest is productive too.", .category = MSG_CATEGORY_EVENING},
    {.text = "Tomorrow is a new opportunity.", .category = MSG_CATEGORY_EVENING},
    {.text = "Let go of what you couldn't control today.", .category = MSG_CATEGORY_EVENING},
    {.text = "Evening peace. Well done.", .category = MSG_CATEGORY_EVENING},
    
    // Night messages (12AM-6AM)
    {.text = "Rest well. You deserve it.", .category = MSG_CATEGORY_NIGHT},
    {.text = "Let your mind rest now.", .category = MSG_CATEGORY_NIGHT},
    {.text = "Peaceful sleep awaits.", .category = MSG_CATEGORY_NIGHT},
    
    // Universal messages (any time)
    {.text = "You matter.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "This feeling is temporary.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "Be kind to yourself.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "You are not alone.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "Your feelings are valid.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "Small steps count.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "You are stronger than you know.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "It's okay to not be okay.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "Growth takes time.", .category = MSG_CATEGORY_UNIVERSAL},
    {.text = "You deserve compassion.", .category = MSG_CATEGORY_UNIVERSAL},
    
    // Post-breathing messages
    {.text = "Well done. Feel that calm?", .category = MSG_CATEGORY_POST_BREATH},
    {.text = "You can return here anytime.", .category = MSG_CATEGORY_POST_BREATH},
    {.text = "Carry this peace with you.", .category = MSG_CATEGORY_POST_BREATH},
    
    // Post-mood check messages
    {.text = "Thank you for checking in.", .category = MSG_CATEGORY_POST_MOOD},
    {.text = "Awareness is the first step.", .category = MSG_CATEGORY_POST_MOOD},
    {.text = "However you feel, it's okay.", .category = MSG_CATEGORY_POST_MOOD},
    
    // Emergency calm messages
    {.text = "You're safe.", .category = MSG_CATEGORY_EMERGENCY},
    {.text = "Breathe...", .category = MSG_CATEGORY_EMERGENCY},
    {.text = "This will pass.", .category = MSG_CATEGORY_EMERGENCY},
    {.text = "You've gotten through this before.", .category = MSG_CATEGORY_EMERGENCY},
    {.text = "Stay with your breath.", .category = MSG_CATEGORY_EMERGENCY},
    {.text = "You are stronger than this moment.", .category = MSG_CATEGORY_EMERGENCY},
};

#define BUILTIN_MESSAGE_COUNT (sizeof(g_builtin_messages) / sizeof(g_builtin_messages[0]))

// Runtime state
static messages_config_t g_config = {0};
static uint8_t g_daily_count = 0;
static uint32_t g_last_message_time = 0;
static comfort_message_t g_message_history[BUILTIN_MESSAGE_COUNT] = {0};

// Get current hour
static uint8_t get_current_hour(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    return (uint8_t)timeinfo.tm_hour;
}

// Get current timestamp (seconds since epoch)
static uint32_t get_timestamp(void) {
    time_t now;
    time(&now);
    return (uint32_t)now;
}

message_category_t messages_get_category_for_hour(uint8_t hour) {
    if (hour >= 6 && hour < 12) {
        return MSG_CATEGORY_MORNING;
    } else if (hour >= 12 && hour < 18) {
        return MSG_CATEGORY_MIDDAY;
    } else if (hour >= 18 && hour < 24) {
        return MSG_CATEGORY_EVENING;
    } else {
        return MSG_CATEGORY_NIGHT;
    }
}

bool messages_init(void) {
    // Set default configuration
    g_config.enabled = true;
    g_config.min_interval_minutes = DEFAULT_MIN_INTERVAL_MINUTES;
    g_config.show_on_wake = true;
    g_config.show_after_breathing = true;
    g_config.show_after_mood_check = true;
    g_config.max_daily_messages = DEFAULT_MAX_DAILY_MESSAGES;
    
    // Initialize message history from built-in messages
    memcpy(g_message_history, g_builtin_messages, sizeof(g_message_history));
    
    // Reset daily count
    g_daily_count = 0;
    g_last_message_time = 0;
    
    ESP_LOGI(TAG, "Comfort messages initialized (%d messages)", BUILTIN_MESSAGE_COUNT);
    return true;
}

bool messages_display(int8_t force_category) {
    if (!g_config.enabled) {
        ESP_LOGD(TAG, "Messages disabled");
        return false;
    }
    
    // Check daily limit
    if (g_daily_count >= g_config.max_daily_messages) {
        ESP_LOGD(TAG, "Daily message limit reached");
        return false;
    }
    
    // Check minimum interval
    uint32_t now = get_timestamp();
    if (g_last_message_time > 0) {
        uint32_t elapsed_minutes = (now - g_last_message_time) / 60;
        if (elapsed_minutes < g_config.min_interval_minutes) {
            ESP_LOGD(TAG, "Too soon since last message (%" PRIu32 " min)", elapsed_minutes);
            return false;
        }
    }
    
    // Determine category
    message_category_t category;
    if (force_category >= 0 && force_category < MSG_CATEGORY_COUNT) {
        category = (message_category_t)force_category;
    } else {
        category = messages_get_category_for_hour(get_current_hour());
    }
    
    // Find available message in category
    int selected_index = -1;
    uint32_t oldest_time = 0;
    
    for (int i = 0; i < BUILTIN_MESSAGE_COUNT; i++) {
        if (g_message_history[i].category == category) {
            // Check if this message was shown recently (avoid repeats within 24h)
            uint32_t time_since_show = now - g_message_history[i].last_shown;
            if (time_since_show > (24 * 3600)) {  // 24 hours
                // Prefer messages that haven't been shown recently
                if (selected_index < 0 || g_message_history[i].last_shown < oldest_time) {
                    selected_index = i;
                    oldest_time = g_message_history[i].last_shown;
                }
            }
        }
    }
    
    // Fallback to universal if no category-specific message available
    if (selected_index < 0) {
        for (int i = 0; i < BUILTIN_MESSAGE_COUNT; i++) {
            if (g_message_history[i].category == MSG_CATEGORY_UNIVERSAL) {
                uint32_t time_since_show = now - g_message_history[i].last_shown;
                if (time_since_show > (24 * 3600)) {
                    if (selected_index < 0 || g_message_history[i].last_shown < oldest_time) {
                        selected_index = i;
                        oldest_time = g_message_history[i].last_shown;
                    }
                }
            }
        }
    }
    
    // If still no message, pick least recently shown
    if (selected_index < 0) {
        oldest_time = 0;
        for (int i = 0; i < BUILTIN_MESSAGE_COUNT; i++) {
            if (g_message_history[i].last_shown < oldest_time || oldest_time == 0) {
                selected_index = i;
                oldest_time = g_message_history[i].last_shown;
            }
        }
    }
    
    if (selected_index < 0) {
        ESP_LOGW(TAG, "No messages available");
        return false;
    }
    
    // Display the message
    const char* message_text = g_message_history[selected_index].text;
    ESP_LOGI(TAG, "Displaying message: \"%s\"", message_text);
    
    // TODO: Call display function
    // display_show_message(message_text, MESSAGE_DISPLAY_DURATION_MS);
    
    // Update tracking
    g_message_history[selected_index].last_shown = now;
    g_message_history[selected_index].show_count++;
    g_last_message_time = now;
    g_daily_count++;
    
    return true;
}

const char* messages_get_current(int8_t* category) {
    // Similar logic to messages_display but doesn't update counters
    message_category_t cat;
    if (*category >= 0 && *category < MSG_CATEGORY_COUNT) {
        cat = (message_category_t)*category;
    } else {
        cat = messages_get_category_for_hour(get_current_hour());
    }
    
    int selected_index = -1;
    uint32_t oldest_time = 0;
    
    for (int i = 0; i < BUILTIN_MESSAGE_COUNT; i++) {
        if (g_message_history[i].category == cat) {
            if (selected_index < 0 || g_message_history[i].last_shown < oldest_time) {
                selected_index = i;
                oldest_time = g_message_history[i].last_shown;
            }
        }
    }
    
    if (selected_index >= 0) {
        if (category) *category = (int8_t)g_message_history[selected_index].category;
        return g_message_history[selected_index].text;
    }
    
    return NULL;
}

void messages_mark_shown(const comfort_message_t* message) {
    if (!message) return;
    
    uint32_t now = get_timestamp();
    for (int i = 0; i < BUILTIN_MESSAGE_COUNT; i++) {
        if (g_message_history[i].text == message->text) {
            g_message_history[i].last_shown = now;
            g_message_history[i].show_count++;
            g_last_message_time = now;
            g_daily_count++;
            break;
        }
    }
}

bool messages_add_custom(const char* text, message_category_t category) {
    // TODO: Implement custom message storage in SPIFFS
    // For now, this is a placeholder
    ESP_LOGW(TAG, "Custom messages not yet implemented");
    return false;
}

uint16_t messages_get_count(void) {
    return BUILTIN_MESSAGE_COUNT;
}

void messages_reset_history(void) {
    for (int i = 0; i < BUILTIN_MESSAGE_COUNT; i++) {
        g_message_history[i].last_shown = 0;
        g_message_history[i].show_count = 0;
    }
    g_daily_count = 0;
    g_last_message_time = 0;
    ESP_LOGI(TAG, "Message history reset");
}

messages_config_t messages_get_config(void) {
    return g_config;
}

void messages_set_config(messages_config_t config) {
    g_config = config;
    ESP_LOGI(TAG, "Messages config updated");
}
