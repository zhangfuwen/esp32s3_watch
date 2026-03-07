/**
 * @file emergency.h
 * @brief Emergency Calm Button - Emotional Comfort Feature
 * 
 * Provides immediate access to calming support during
 * panic, anxiety, or high stress moments.
 */

#ifndef EMERGENCY_H
#define EMERGENCY_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Emergency calm states
 */
typedef enum {
    EMERGENCY_STATE_IDLE,        // Not active
    EMERGENCY_STATE_ACTIVE,      // Emergency calm active
    EMERGENCY_STATE_COOLDOWN     // Just finished (prevent re-trigger)
} emergency_state_t;

/**
 * @brief Emergency calm configuration
 */
typedef struct {
    bool enabled;                     // Enable emergency calm feature
    uint8_t activation_duration_ms;   // How long to hold both buttons (default: 1000)
    uint8_t auto_exit_minutes;        // Auto-exit after X minutes (default: 2)
    bool haptic_enabled;              // Enable haptic feedback
    bool audio_enabled;               // Enable soothing audio (future)
    uint8_t cooldown_minutes;         // Cooldown period after use
} emergency_config_t;

/**
 * @brief Initialize emergency calm feature
 * 
 * @return true if initialization successful
 */
bool emergency_init(void);

/**
 * @brief Check and handle emergency activation
 * 
 * Call this in the main input handling loop.
 * Checks for simultaneous button press.
 * 
 * @param button1_pressed First button state
 * @param button2_pressed Second button state
 * @return true if emergency mode was activated
 */
bool emergency_check_activation(bool button1_pressed, bool button2_pressed);

/**
 * @brief Update emergency calm display and state
 * 
 * Call this regularly in main loop when emergency mode is active.
 * 
 * @return true if still in emergency mode
 */
bool emergency_update(void);

/**
 * @brief Get current emergency state
 * 
 * @return Current state
 */
emergency_state_t emergency_get_state(void);

/**
 * @brief Manually activate emergency calm
 * 
 * Can be called from other features (e.g., low mood detection).
 */
void emergency_activate(void);

/**
 * @brief Deactivate emergency calm
 * 
 * Exits emergency mode gracefully.
 */
void emergency_deactivate(void);

/**
 * @brief Check if emergency mode is active
 * 
 * @return true if active
 */
bool emergency_is_active(void);

/**
 * @brief Get emergency configuration
 * 
 * @return Current configuration
 */
emergency_config_t emergency_get_config(void);

/**
 * @brief Set emergency configuration
 * 
 * @param config New configuration
 */
void emergency_set_config(emergency_config_t config);

/**
 * @brief Get cooldown remaining (seconds)
 * 
 * @return Seconds remaining in cooldown, 0 if no cooldown
 */
uint32_t emergency_get_cooldown_remaining(void);

#endif // EMERGENCY_H
