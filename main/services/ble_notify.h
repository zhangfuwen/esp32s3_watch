/**
 * @file ble_notify.h
 * @brief BLE Notification Service
 */

#ifndef BLE_NOTIFY_H
#define BLE_NOTIFY_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_DEVICE_NAME         "ESP32-Watch"
#define BLE_MAX_NOTIFY_LEN      200

// Callback for incoming notifications
typedef void (*ble_notify_callback_t)(const char *message, uint8_t len);

// Public API
esp_err_t ble_notify_init(ble_notify_callback_t callback);
esp_err_t ble_notify_deinit(void);
esp_err_t ble_notify_start_advertising(void);
esp_err_t ble_notify_stop_advertising(void);
bool ble_notify_is_connected(void);
esp_err_t ble_notify_send_message(const char *message);

#ifdef __cplusplus
}
#endif

#endif // BLE_NOTIFY_H
