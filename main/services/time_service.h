/**
 * @file time_service.h
 * @brief Time Service - RTC and NTP synchronization
 */

#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "event_bus.h"

typedef enum {
    TIME_SOURCE_RTC,
    TIME_SOURCE_NTP,
    TIME_SOURCE_BLE,
    TIME_SOURCE_MANUAL,
} time_source_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
    int32_t timezone_offset;
} datetime_t;

typedef void (*time_sync_callback_t)(time_source_t source);

typedef struct {
    bool enable_ntp;
    bool enable_ble_time;
    bool enable_ble_notifications;
    char ntp_server[64];
    int8_t timezone_hours;
    int8_t timezone_minutes;
    bool is_dst;
} time_config_t;

esp_err_t time_service_init(const time_config_t *config);

esp_err_t time_service_deinit(void);

esp_err_t time_service_sync_ntp(void);

esp_err_t time_service_set_time(const datetime_t *datetime);

esp_err_t time_service_get_time(datetime_t *datetime);

esp_err_t time_service_set_timezone(int8_t hours, int8_t minutes);

esp_err_t time_service_enable_auto_sync(bool enable);

uint32_t time_service_get_timestamp(void);

time_source_t time_service_get_last_source(void);

esp_err_t time_service_register_callback(time_sync_callback_t callback);

esp_err_t time_service_set_alarm(uint8_t hour, uint8_t minute, bool repeat_daily);

esp_err_t time_service_cancel_alarm(void);

bool time_service_is_dst(void);

esp_err_t time_service_set_dst(bool enable);

#endif // TIME_SERVICE_H
