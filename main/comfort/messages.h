/**
 * @file messages.h
 * @brief Comfort Messages - Emotional Comfort Feature
 * 
 * Displays encouraging and supportive messages to provide
 * emotional comfort throughout the day.
 */

#ifndef COMFORT_MESSAGES_H
#define COMFORT_MESSAGES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Message categories
 */
typedef enum {
    MSG_CATEGORY_MORNING,      // Morning messages (6AM-12PM)
    MSG_CATEGORY_MIDDAY,       // Midday messages (12PM-6PM)
    MSG_CATEGORY_EVENING,      // Evening messages (6PM-12AM)
    MSG_CATEGORY_NIGHT,        // Night messages (12AM-6AM)
    MSG_CATEGORY_UNIVERSAL,    // Any time messages
    MSG_CATEGORY_POST_BREATH,  // After breathing session
    MSG_CATEGORY_POST_MOOD,    // After mood check-in
    MSG_CATEGORY_EMERGENCY,    // Emergency calm messages
    MSG_CATEGORY_COUNT
} message_category_t;

/**
 * @brief Message structure
 */
typedef struct {
    const char* text;           // Message text (stored in flash)
    message_category_t category; // Message category
    uint32_t last_shown;        // Last time shown (timestamp)
    uint8_t show_count;         // How many times shown
} comfort_message_t;

/**
 * @brief Message display configuration
 */
typedef struct {
    bool enabled;                       // Enable comfort messages
    uint32_t min_interval_minutes;      // Minimum time between messages
    bool show_on_wake;                  // Show message on wrist wake
    bool show_after_breathing;          // Show after breathing session
    bool show_after_mood_check;         // Show after mood check-in
    uint8_t max_daily_messages;         // Maximum messages per day
} messages_config_t;

/**
 * @brief Initialize comfort messages system
 * 
 * Loads messages from flash and initializes state.
 * 
 * @return true if initialization successful
 */
bool messages_init(void);

/**
 * @brief Display a comfort message
 * 
 * Automatically selects appropriate message based on
 * time of day and context.
 * 
 * @param force_category Force specific category (or -1 for auto)
 * @return true if message displayed
 */
bool messages_display(int8_t force_category);

/**
 * @brief Get message for current context
 * 
 * @param category Output: selected message category
 * @return Message text (NULL if none available)
 */
const char* messages_get_current(int8_t* category);

/**
 * @brief Mark message as shown
 * 
 * Updates last_shown timestamp and show_count.
 * 
 * @param message Message that was shown
 */
void messages_mark_shown(const comfort_message_t* message);

/**
 * @brief Get category based on current hour
 * 
 * @param hour Hour of day (0-23)
 * @return Appropriate message category
 */
message_category_t messages_get_category_for_hour(uint8_t hour);

/**
 * @brief Add custom message
 * 
 * @param text Message text
 * @param category Message category
 * @return true if added successfully
 */
bool messages_add_custom(const char* text, message_category_t category);

/**
 * @brief Get total message count
 * 
 * @return Number of messages available
 */
uint16_t messages_get_count(void);

/**
 * @brief Reset message history
 * 
 * Clears last_shown timestamps (useful for testing).
 */
void messages_reset_history(void);

/**
 * @brief Get messages configuration
 * 
 * @return Current configuration
 */
messages_config_t messages_get_config(void);

/**
 * @brief Set messages configuration
 * 
 * @param config New configuration
 */
void messages_set_config(messages_config_t config);

#endif // COMFORT_MESSAGES_H
