/**
 * @file breathing.h
 * @brief Breathing Companion - Emotional Comfort Feature
 * 
 * Guides users through calming breathing exercises with visual
 * and haptic feedback.
 */

#ifndef BREATHING_H
#define BREATHING_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Breathing pattern types
 */
typedef enum {
    BREATH_PATTERN_478,      // 4-7-8 relaxation technique
    BREATH_PATTERN_BOX,      // Box breathing (4-4-4-4)
    BREATH_PATTERN_COHERENT, // Coherent breathing (5-5)
    BREATH_PATTERN_CUSTOM    // Custom pattern
} breathing_pattern_t;

/**
 * @brief Breathing phase states
 */
typedef enum {
    BREATH_PHASE_IN,         // Inhale
    BREATH_PHASE_HOLD_IN,    // Hold after inhale
    BREATH_PHASE_OUT,        // Exhale
    BREATH_PHASE_HOLD_OUT,   // Hold after exhale
    BREATH_PHASE_IDLE        // Not actively breathing
} breathing_phase_t;

/**
 * @brief Breathing session configuration
 */
typedef struct {
    breathing_pattern_t pattern;
    uint8_t cycles_target;        // Target number of cycles (0 = unlimited)
    uint8_t cycles_completed;     // Completed cycles counter
    bool haptic_enabled;          // Enable haptic feedback
    bool audio_enabled;           // Enable audio feedback (future)
} breathing_config_t;

/**
 * @brief Breathing session state
 */
typedef struct {
    bool active;                  // Is breathing session active?
    breathing_phase_t phase;      // Current phase
    uint32_t phase_start_ms;      // When current phase started
    uint32_t phase_duration_ms;   // Duration of current phase
    uint8_t current_cycle;        // Current cycle number
    float animation_progress;     // 0.0 to 1.0 for animation
} breathing_state_t;

/**
 * @brief Initialize breathing companion
 * 
 * @return true if initialization successful
 */
bool breathing_init(void);

/**
 * @brief Start a breathing session
 * 
 * @param pattern Breathing pattern to use
 * @param cycles Number of cycles (0 for unlimited)
 * @return true if session started successfully
 */
bool breathing_start(breathing_pattern_t pattern, uint8_t cycles);

/**
 * @brief Stop the current breathing session
 */
void breathing_stop(void);

/**
 * @brief Update breathing animation and state
 * 
 * Call this regularly in main loop to update animation
 * and check for phase transitions.
 * 
 * @return true if session is still active
 */
bool breathing_update(void);

/**
 * @brief Get current breathing phase
 * 
 * @return Current breathing phase
 */
breathing_phase_t breathing_get_phase(void);

/**
 * @brief Get animation progress (0.0 to 1.0)
 * 
 * Used for rendering the breathing circle animation.
 * 
 * @return Animation progress
 */
float breathing_get_animation_progress(void);

/**
 * @brief Get current phase text for display
 * 
 * @return Phase text string (e.g., "Breathe In...")
 */
const char* breathing_get_phase_text(void);

/**
 * @brief Handle button press during breathing
 * 
 * @param button_id Button that was pressed
 * @return true if should exit breathing mode
 */
bool breathing_handle_button(uint8_t button_id);

/**
 * @brief Check if breathing session is complete
 * 
 * @return true if session completed all cycles
 */
bool breathing_is_complete(void);

/**
 * @brief Get default pattern for quick start
 * 
 * @return Default breathing pattern
 */
breathing_pattern_t breathing_get_default_pattern(void);

#endif // BREATHING_H
