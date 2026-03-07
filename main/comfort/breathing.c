/**
 * @file breathing.c
 * @brief Breathing Companion Implementation
 * 
 * Guides users through calming breathing exercises with visual
 * and haptic feedback.
 */

#include "breathing.h"
#include "../display.h"
#include "../input.h"
#include <string.h>
#include <stdio.h>

// ESP-IDF includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

// Tag for logging
static const char* TAG = "breathing";

// Default timing values (in milliseconds)
#define TIMING_478_IN       4000
#define TIMING_478_HOLD_IN  7000
#define TIMING_478_OUT      8000
#define TIMING_478_HOLD_OUT 0

#define TIMING_BOX_IN       4000
#define TIMING_BOX_HOLD_IN  4000
#define TIMING_BOX_OUT      4000
#define TIMING_BOX_HOLD_OUT 4000

#define TIMING_COHERENT_IN  5000
#define TIMING_COHERENT_OUT 5000

// Default cycles
#define DEFAULT_CYCLES  3

// Animation update interval (ms)
#define ANIMATION_UPDATE_MS 50

// Static state
static breathing_state_t g_state = {0};
static breathing_config_t g_config = {0};
static breathing_pattern_t g_current_pattern = BREATH_PATTERN_478;

// Phase timing lookup
static void get_pattern_timing(breathing_pattern_t pattern, 
                                uint32_t* in, uint32_t* hold_in, 
                                uint32_t* out, uint32_t* hold_out) {
    switch (pattern) {
        case BREATH_PATTERN_478:
            *in = TIMING_478_IN;
            *hold_in = TIMING_478_HOLD_IN;
            *out = TIMING_478_OUT;
            *hold_out = TIMING_478_HOLD_OUT;
            break;
        
        case BREATH_PATTERN_BOX:
            *in = TIMING_BOX_IN;
            *hold_in = TIMING_BOX_HOLD_IN;
            *out = TIMING_BOX_OUT;
            *hold_out = TIMING_BOX_HOLD_OUT;
            break;
        
        case BREATH_PATTERN_COHERENT:
            *in = TIMING_COHERENT_IN;
            *hold_in = 0;
            *out = TIMING_COHERENT_OUT;
            *hold_out = 0;
            break;
        
        default:
            *in = TIMING_478_IN;
            *hold_in = 0;
            *out = TIMING_478_OUT;
            *hold_out = 0;
            break;
    }
}

// Get phase text
static const char* get_phase_text(breathing_phase_t phase) {
    switch (phase) {
        case BREATH_PHASE_IN:
            return "Breathe In...";
        case BREATH_PHASE_HOLD_IN:
            return "Hold...";
        case BREATH_PHASE_OUT:
            return "Breathe Out...";
        case BREATH_PHASE_HOLD_OUT:
            return "Hold...";
        default:
            return "";
    }
}

// Trigger haptic feedback for phase
static void haptic_for_phase(breathing_phase_t phase) {
    // TODO: Implement haptic feedback
    // For now, this is a placeholder
    // vibration_motor_pulse(phase == BREATH_PHASE_IN ? 100 : 50);
}

// Draw breathing animation
static void draw_breathing_animation(void) {
    // Get animation parameters based on phase
    float progress = g_state.animation_progress;
    float radius_scale = 0.0f;
    
    switch (g_state.phase) {
        case BREATH_PHASE_IN:
            // Circle expands during inhale
            radius_scale = 0.3f + (0.7f * progress);
            break;
        
        case BREATH_PHASE_HOLD_IN:
            // Circle stays large during hold
            radius_scale = 1.0f;
            break;
        
        case BREATH_PHASE_OUT:
            // Circle contracts during exhale
            radius_scale = 1.0f - (0.7f * progress);
            break;
        
        case BREATH_PHASE_HOLD_OUT:
            // Circle stays small during hold
            radius_scale = 0.3f;
            break;
        
        default:
            radius_scale = 0.5f;
            break;
    }
    
    // TODO: Call display functions to render breathing circle
    // display_draw_breathing_circle(radius_scale, g_state.phase);
    
    // For now, just log
    // ESP_LOGD(TAG, "Breathing animation: phase=%d, progress=%.2f, scale=%.2f", 
    //          g_state.phase, progress, radius_scale);
}

// Transition to next phase
static void next_phase(void) {
    uint32_t in, hold_in, out, hold_out;
    get_pattern_timing(g_current_pattern, &in, &hold_in, &out, &hold_out);
    
    switch (g_state.phase) {
        case BREATH_PHASE_IN:
            if (hold_in > 0) {
                g_state.phase = BREATH_PHASE_HOLD_IN;
                g_state.phase_duration_ms = hold_in;
            } else {
                g_state.phase = BREATH_PHASE_OUT;
                g_state.phase_duration_ms = out;
            }
            break;
        
        case BREATH_PHASE_HOLD_IN:
            g_state.phase = BREATH_PHASE_OUT;
            g_state.phase_duration_ms = out;
            break;
        
        case BREATH_PHASE_OUT:
            if (hold_out > 0) {
                g_state.phase = BREATH_PHASE_HOLD_OUT;
                g_state.phase_duration_ms = hold_out;
            } else {
                // Cycle complete
                g_state.current_cycle++;
                g_state.phase = BREATH_PHASE_IN;
                g_state.phase_duration_ms = in;
                
                // Check if session complete
                if (g_config.cycles_target > 0 && 
                    g_state.current_cycle >= g_config.cycles_target) {
                    // Session complete - don't restart
                }
            }
            break;
        
        case BREATH_PHASE_HOLD_OUT:
            g_state.current_cycle++;
            g_state.phase = BREATH_PHASE_IN;
            g_state.phase_duration_ms = in;
            
            // Check if session complete
            if (g_config.cycles_target > 0 && 
                g_state.current_cycle >= g_config.cycles_target) {
                // Session complete
            }
            break;
        
        default:
            g_state.phase = BREATH_PHASE_IN;
            g_state.phase_duration_ms = in;
            break;
    }
    
    g_state.phase_start_ms = (uint32_t)(esp_timer_get_time() / 1000);
    g_state.animation_progress = 0.0f;
    
    // Trigger haptic for new phase
    haptic_for_phase(g_state.phase);
}

bool breathing_init(void) {
    // Reset state
    memset(&g_state, 0, sizeof(g_state));
    memset(&g_config, 0, sizeof(g_config));
    
    // Set defaults
    g_config.pattern = BREATH_PATTERN_478;
    g_config.cycles_target = DEFAULT_CYCLES;
    g_config.haptic_enabled = true;
    g_config.audio_enabled = false;
    
    g_current_pattern = BREATH_PATTERN_478;
    
    ESP_LOGI(TAG, "Breathing companion initialized");
    return true;
}

bool breathing_start(breathing_pattern_t pattern, uint8_t cycles) {
    if (g_state.active) {
        ESP_LOGW(TAG, "Breathing session already active");
        return false;
    }
    
    g_current_pattern = pattern;
    g_config.cycles_target = (cycles > 0) ? cycles : DEFAULT_CYCLES;
    g_config.pattern = pattern;
    
    // Initialize state
    g_state.active = true;
    g_state.phase = BREATH_PHASE_IN;
    g_state.current_cycle = 0;
    g_state.cycles_completed = 0;
    g_state.animation_progress = 0.0f;
    
    // Get initial timing
    uint32_t in, hold_in, out, hold_out;
    get_pattern_timing(pattern, &in, &hold_in, &out, &hold_out);
    g_state.phase_duration_ms = in;
    g_state.phase_start_ms = (uint32_t)(esp_timer_get_time() / 1000);
    
    // Initial haptic
    haptic_for_phase(BREATH_PHASE_IN);
    
    ESP_LOGI(TAG, "Breathing session started: pattern=%d, cycles=%d", pattern, cycles);
    return true;
}

void breathing_stop(void) {
    if (!g_state.active) {
        return;
    }
    
    g_state.active = false;
    g_state.phase = BREATH_PHASE_IDLE;
    g_state.cycles_completed = g_state.current_cycle;
    
    ESP_LOGI(TAG, "Breathing session stopped. Completed cycles: %d", g_state.cycles_completed);
}

bool breathing_update(void) {
    if (!g_state.active) {
        return false;
    }
    
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t elapsed = now - g_state.phase_start_ms;
    
    // Update animation progress
    if (g_state.phase_duration_ms > 0) {
        g_state.animation_progress = (float)elapsed / (float)g_state.phase_duration_ms;
        if (g_state.animation_progress > 1.0f) {
            g_state.animation_progress = 1.0f;
        }
    } else {
        g_state.animation_progress = 1.0f;
    }
    
    // Draw animation
    draw_breathing_animation();
    
    // Check for phase transition
    if (elapsed >= g_state.phase_duration_ms) {
        next_phase();
    }
    
    // Check for session completion
    if (g_config.cycles_target > 0 && 
        g_state.current_cycle >= g_config.cycles_target &&
        g_state.phase == BREATH_PHASE_IN) {
        // Session complete
        g_state.active = false;
        ESP_LOGI(TAG, "Breathing session complete: %d cycles", g_state.cycles_completed);
        return false;
    }
    
    return true;
}

breathing_phase_t breathing_get_phase(void) {
    return g_state.phase;
}

float breathing_get_animation_progress(void) {
    return g_state.animation_progress;
}

const char* breathing_get_phase_text(void) {
    return get_phase_text(g_state.phase);
}

bool breathing_handle_button(uint8_t button_id) {
    // Any button press exits breathing mode
    // This provides simple, intuitive control
    return true; // Exit breathing mode
}

bool breathing_is_complete(void) {
    return !g_state.active && g_state.cycles_completed > 0;
}

breathing_pattern_t breathing_get_default_pattern(void) {
    return BREATH_PATTERN_478; // 4-7-8 is most relaxing
}
