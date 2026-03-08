/**
 * @file watch_face.h
 * @brief Watch Face UI - Main display interface
 */

#ifndef WATCH_FACE_H
#define WATCH_FACE_H

#include <stdint.h>
#include "driver/gpio.h"
#include "event_bus.h"

typedef enum {
    WATCH_FACE_DIGITAL,     // Digital clock
    WATCH_FACE_ANALOG,      // Analog clock
    WATCH_FACE_MINIMAL,     // Minimal (time only, low power)
    WATCH_FACE_INFO,        // Info screen (battery, steps, etc.)
} watch_face_style_t;

typedef struct {
    watch_face_style_t style;
    bool show_seconds;
    bool show_date;
    bool show_battery;
    bool show_steps;
    uint8_t brightness;     // 0-100%
} watch_face_config_t;

/**
 * @brief Initialize watch face
 * @param config Watch face configuration
 * @return ESP_OK on success
 */
esp_err_t watch_face_init(const watch_face_config_t *config);

/**
 * @brief Start watch face display task
 * @return ESP_OK on success
 */
esp_err_t watch_face_start(void);

/**
 * @brief Stop watch face display task
 * @return ESP_OK on success
 */
esp_err_t watch_face_stop(void);

/**
 * @brief Update watch face display with time
 * @param datetime Current datetime
 * @return ESP_OK on success
 */
esp_err_t watch_face_update(datetime_t *datetime);

/**
 * @brief Show notification on watch face
 * @param title Notification title
 * @param body Notification body
 * @return ESP_OK on success
 */
esp_err_t watch_face_show_notification(const char *title, const char *body);

/**
 * @brief Set watch face style
 * @param style New style
 * @return ESP_OK on success
 */
esp_err_t watch_face_set_style(watch_face_style_t style);

/**
 * @brief Get current watch face style
 * @return Current style
 */
watch_face_style_t watch_face_get_style(void);

/**
 * @brief Set display brightness
 * @param brightness 0-100%
 * @return ESP_OK on success
 */
esp_err_t watch_face_set_brightness(uint8_t brightness);

/**
 * @brief Handle button press event
 * @param button GPIO number of pressed button
 */
void watch_face_handle_button_press(gpio_num_t button);

/**
 * @brief Handle wrist raise event (turn on display)
 */
void watch_face_handle_wrist_raise(void);

/**
 * @brief Handle notification event
 * @param notification Notification data
 */
void watch_face_handle_notification(const event_t *notification);

#endif // WATCH_FACE_H
