/**
 * @file emergency.c
 * @brief Emergency Calm Button Implementation
 * 
 * Provides immediate access to calming support during
 * panic, anxiety, or high stress moments.
 */

#include "emergency.h"
#include "../display.h"
#include "../input.h"
#include <string.h>
#include <stdio.h>

// ESP-IDF includes
#include "esp_log.h"
#include "esp_timer.h"

// Tag for logging
static const char* TAG = "emergency";

// Default configuration
#define DEFAULT_ACTIVATION_DURATION_MS  1000    // Hold both buttons for 1 second
#define DEFAULT_AUTO_EXIT_MINUTES       2       // Auto-exit after 2 minutes
#define DEFAULT_COOLDOWN_MINUTES        5       // 5 minute cooldown after use

// Animation update interval (ms)
#define ANIMATION_UPDATE_MS  100

// Emergency messages (calming, grounding)
static const char* g_emergency_messages[] = {
    "You're safe.",
    "Breathe...",
    "This will pass.",
    "You've gotten through this before.",
    "Stay with your breath.",
    "You are stronger than this moment.",
    "Feel your feet on the ground.",
    "You are here. You are now.",
    "One breath at a time.",
    "This feeling is temporary."
};

#define EMERGENCY_MESSAGE_COUNT (sizeof(g_emergency_messages) / sizeof(g_emergency_messages[0]))

// Static state
static emergency_state_t g_state = EMERGENCY_STATE_IDLE;
static emergency_config_t g_config = {0};
static uint32_t g_activation_start_ms = 0;
static uint32_t g_emergency_start_ms = 0;
static uint32_t g_cooldown_end_ms = 0;
static uint8_t g_current_message_index = 0;
static float g_animation_phase = 0.0f;

// Get current time in milliseconds
static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// Draw emergency calm screen
static void draw_emergency_screen(void) {
    // Calculate animation phase (slow pulsing circle)
    uint32_t elapsed = get_time_ms() - g_emergency_start_ms;
    g_animation_phase = 0.5f + 0.5f * sinf((float)elapsed / 2000.0f * 3.14159f);
    
    // Get current message (rotate every 10 seconds)
    uint32_t message_index = (elapsed / 10000) % EMERGENCY_MESSAGE_COUNT;
    const char* message = g_emergency_messages[message_index];
    
    // TODO: Call display functions
    // display_draw_emergency_calm(g_animation_phase, message);
    
    ESP_LOGD(TAG, "Emergency screen: phase=%.2f, message=\"%s\"", g_animation_phase, message);
}

// Trigger calming haptic pattern
static void haptic_calming_pattern(void) {
    // TODO: Implement gentle, rhythmic haptic
    // vibration_motor_pattern(VIBRATION_PATTERN_CALM);
    ESP_LOGD(TAG, "Calming haptic pattern");
}

bool emergency_init(void) {
    // Set default configuration
    g_config.enabled = true;
    g_config.activation_duration_ms = DEFAULT_ACTIVATION_DURATION_MS;
    g_config.auto_exit_minutes = DEFAULT_AUTO_EXIT_MINUTES;
    g_config.haptic_enabled = true;
    g_config.audio_enabled = false;
    g_config.cooldown_minutes = DEFAULT_COOLDOWN_MINUTES;
    
    g_state = EMERGENCY_STATE_IDLE;
    g_activation_start_ms = 0;
    g_emergency_start_ms = 0;
    g_cooldown_end_ms = 0;
    g_current_message_index = 0;
    g_animation_phase = 0.0f;
    
    ESP_LOGI(TAG, "Emergency calm initialized");
    return true;
}

bool emergency_check_activation(bool button1_pressed, bool button2_pressed) {
    // Check if feature is enabled
    if (!g_config.enabled) {
        return false;
    }
    
    // Check if in cooldown
    if (g_state == EMERGENCY_STATE_COOLDOWN) {
        uint32_t now = get_time_ms();
        if (now >= g_cooldown_end_ms) {
            g_state = EMERGENCY_STATE_IDLE;
            ESP_LOGD(TAG, "Emergency cooldown ended");
        } else {
            return false;  // Still in cooldown
        }
    }
    
    // Check if already active
    if (g_state == EMERGENCY_STATE_ACTIVE) {
        return false;
    }
    
    // Check for simultaneous button press
    if (button1_pressed && button2_pressed) {
        uint32_t now = get_time_ms();
        
        if (g_activation_start_ms == 0) {
            // Start timing
            g_activation_start_ms = now;
            ESP_LOGD(TAG, "Button hold started");
        } else {
            // Check if held long enough
            uint32_t hold_duration = now - g_activation_start_ms;
            if (hold_duration >= g_config.activation_duration_ms) {
                // Activate emergency mode!
                emergency_activate();
                return true;
            }
        }
    } else {
        // Buttons released before duration
        g_activation_start_ms = 0;
    }
    
    return false;
}

void emergency_activate(void) {
    if (g_state == EMERGENCY_STATE_ACTIVE) {
        return;  // Already active
    }
    
    g_state = EMERGENCY_STATE_ACTIVE;
    g_emergency_start_ms = get_time_ms();
    g_activation_start_ms = 0;
    g_current_message_index = 0;
    g_animation_phase = 0.0f;
    
    // Initial haptic feedback
    if (g_config.haptic_enabled) {
        haptic_calming_pattern();
    }
    
    ESP_LOGW(TAG, "EMERGENCY CALM ACTIVATED");
}

void emergency_deactivate(void) {
    if (g_state != EMERGENCY_STATE_ACTIVE) {
        return;
    }
    
    g_state = EMERGENCY_STATE_COOLDOWN;
    g_cooldown_end_ms = get_time_ms() + (g_config.cooldown_minutes * 60 * 1000);
    
    ESP_LOGI(TAG, "Emergency calm deactivated. Cooldown: %d minutes", g_config.cooldown_minutes);
}

bool emergency_update(void) {
    if (g_state != EMERGENCY_STATE_ACTIVE) {
        return false;
    }
    
    uint32_t now = get_time_ms();
    uint32_t elapsed = now - g_emergency_start_ms;
    uint32_t auto_exit_ms = g_config.auto_exit_minutes * 60 * 1000;
    
    // Update display
    draw_emergency_screen();
    
    // Check for auto-exit
    if (elapsed >= auto_exit_ms) {
        emergency_deactivate();
        return false;
    }
    
    return true;
}

emergency_state_t emergency_get_state(void) {
    return g_state;
}

bool emergency_is_active(void) {
    return g_state == EMERGENCY_STATE_ACTIVE;
}

uint32_t emergency_get_cooldown_remaining(void) {
    if (g_state != EMERGENCY_STATE_COOLDOWN) {
        return 0;
    }
    
    uint32_t now = get_time_ms();
    if (now >= g_cooldown_end_ms) {
        return 0;
    }
    
    return (g_cooldown_end_ms - now) / 1000;  // Convert to seconds
}

emergency_config_t emergency_get_config(void) {
    return g_config;
}

void emergency_set_config(emergency_config_t config) {
    g_config = config;
    ESP_LOGI(TAG, "Emergency config updated");
}
