/**
 * @file time_service.c
 * @brief Time Service - RTC and NTP synchronization
 */

#include "time_service.h"
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "lwip/apps/sntp.h"

static const char *TAG = "TIME_SERVICE";

static datetime_t current_time = {0};
static time_source_t last_sync_source = TIME_SOURCE_RTC;
static time_config_t config = {0};
static time_sync_callback_t sync_callback = NULL;
static esp_timer_handle_t ntp_timer_handle = NULL;
static bool auto_sync_enabled = false;

// RTC memory for time persistence across deep sleep
RTC_DATA_ATTR static uint64_t rtc_time_seconds = 0;
RTC_DATA_ATTR static bool rtc_time_valid = false;

static void time_sync_cb(struct timeval *tv) {
    time_t now = tv->tv_sec;
    struct tm timeinfo = {0};
    localtime_r(&now, &timeinfo);
    
    current_time.year = timeinfo.tm_year + 1900;
    current_time.month = timeinfo.tm_mon + 1;
    current_time.day = timeinfo.tm_mday;
    current_time.hour = timeinfo.tm_hour;
    current_time.minute = timeinfo.tm_min;
    current_time.second = timeinfo.tm_sec;
    current_time.weekday = timeinfo.tm_wday;
    
    last_sync_source = TIME_SOURCE_NTP;
    rtc_time_seconds = (uint64_t)now;
    rtc_time_valid = true;
    
    ESP_LOGI(TAG, "Time synchronized via NTP: %04d-%02d-%02d %02d:%02d:%02d",
             current_time.year, current_time.month, current_time.day,
             current_time.hour, current_time.minute, current_time.second);
    
    // Publish time update event
    event_t event = {.type = EVENT_BLE_SYNC_TIME, .data_u32 = 0};
    event_bus_publish(&event);
    
    if (sync_callback) {
        sync_callback(TIME_SOURCE_NTP);
    }
}

static void ntp_timer_callback(void *arg) {
    if (auto_sync_enabled && config.enable_ntp) {
        time_service_sync_ntp();
    }
}

esp_err_t time_service_init(const time_config_t *cfg) {
    if (!cfg) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&config, cfg, sizeof(time_config_t));
    
    // Initialize SNTP
    if (config.enable_ntp) {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, config.ntp_server[0] ? config.ntp_server : "pool.ntp.org");
        sntp_setservername(1, "time.apple.com");
        sntp_setservername(2, "time.google.com");
        sntp_set_time_sync_notification_cb(time_sync_cb);
        
        // Set timezone
        char tz_str[64];
        snprintf(tz_str, sizeof(tz_str), "UTC%d:%d", config.timezone_hours, config.timezone_minutes);
        setenv("TZ", tz_str, 1);
        tzset();
        
        sntp_init();
        ESP_LOGI(TAG, "SNTP initialized with server: %s", config.ntp_server);
    }
    
    // Restore time from RTC memory after deep sleep
    if (rtc_time_valid) {
        time_t now = (time_t)rtc_time_seconds;
        struct tm timeinfo = {0};
        localtime_r(&now, &timeinfo);
        
        current_time.year = timeinfo.tm_year + 1900;
        current_time.month = timeinfo.tm_mon + 1;
        current_time.day = timeinfo.tm_mday;
        current_time.hour = timeinfo.tm_hour;
        current_time.minute = timeinfo.tm_min;
        current_time.second = timeinfo.tm_sec;
        current_time.weekday = timeinfo.tm_wday;
        
        ESP_LOGI(TAG, "Time restored from RTC memory");
        last_sync_source = TIME_SOURCE_RTC;
    }
    
    // Create NTP sync timer (sync every hour)
    const esp_timer_create_args_t ntp_timer_args = {
        .callback = ntp_timer_callback,
        .name = "ntp_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&ntp_timer_args, &ntp_timer_handle));
    
    return ESP_OK;
}

esp_err_t time_service_deinit(void) {
    if (ntp_timer_handle) {
        esp_timer_stop(ntp_timer_handle);
        esp_timer_delete(ntp_timer_handle);
        ntp_timer_handle = NULL;
    }
    
    if (config.enable_ntp) {
        sntp_stop();
    }
    
    return ESP_OK;
}

esp_err_t time_service_sync_ntp(void) {
    if (!config.enable_ntp) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Syncing time via NTP...");
    sntp_restart();
    
    // Wait for sync (max 10 seconds)
    for (int i = 0; i < 10; i++) {
        if (last_sync_source == TIME_SOURCE_NTP) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGW(TAG, "NTP sync timeout");
    return ESP_ERR_TIMEOUT;
}

esp_err_t time_service_set_time(const datetime_t *datetime) {
    if (!datetime) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct tm timeinfo = {0};
    timeinfo.tm_year = datetime->year - 1900;
    timeinfo.tm_mon = datetime->month - 1;
    timeinfo.tm_mday = datetime->day;
    timeinfo.tm_hour = datetime->hour;
    timeinfo.tm_min = datetime->minute;
    timeinfo.tm_sec = datetime->second;
    timeinfo.tm_wday = datetime->weekday;
    
    time_t t = mktime(&timeinfo);
    struct timeval tv = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&tv, NULL);
    
    memcpy(&current_time, datetime, sizeof(datetime_t));
    rtc_time_seconds = (uint64_t)t;
    rtc_time_valid = true;
    last_sync_source = TIME_SOURCE_MANUAL;
    
    ESP_LOGI(TAG, "Time set manually: %04d-%02d-%02d %02d:%02d:%02d",
             datetime->year, datetime->month, datetime->day,
             datetime->hour, datetime->minute, datetime->second);
    
    return ESP_OK;
}

esp_err_t time_service_get_time(datetime_t *datetime) {
    if (!datetime) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Get current time from system
    time_t now;
    struct tm timeinfo = {0};
    time(&now);
    localtime_r(&now, &timeinfo);
    
    datetime->year = timeinfo.tm_year + 1900;
    datetime->month = timeinfo.tm_mon + 1;
    datetime->day = timeinfo.tm_mday;
    datetime->hour = timeinfo.tm_hour;
    datetime->minute = timeinfo.tm_min;
    datetime->second = timeinfo.tm_sec;
    datetime->weekday = timeinfo.tm_wday;
    
    // If RTC time is valid and system time is not synced, use RTC
    if (rtc_time_valid && last_sync_source == TIME_SOURCE_RTC) {
        // RTC time is already in current_time
        memcpy(datetime, &current_time, sizeof(datetime_t));
    }
    
    return ESP_OK;
}

esp_err_t time_service_set_timezone(int8_t hours, int8_t minutes) {
    config.timezone_hours = hours;
    config.timezone_minutes = minutes;
    
    char tz_str[64];
    snprintf(tz_str, sizeof(tz_str), "UTC%d:%d", hours, minutes);
    setenv("TZ", tz_str, 1);
    tzset();
    
    ESP_LOGI(TAG, "Timezone set to UTC%d:%d", hours, minutes);
    return ESP_OK;
}

esp_err_t time_service_enable_auto_sync(bool enable) {
    auto_sync_enabled = enable;
    
    if (enable && ntp_timer_handle) {
        // Sync every hour (3600000000 microseconds)
        esp_timer_start_periodic(ntp_timer_handle, 3600000000);
        ESP_LOGI(TAG, "Auto NTP sync enabled (every hour)");
    } else if (ntp_timer_handle) {
        esp_timer_stop(ntp_timer_handle);
        ESP_LOGI(TAG, "Auto NTP sync disabled");
    }
    
    return ESP_OK;
}

uint32_t time_service_get_timestamp(void) {
    time_t now;
    time(&now);
    return (uint32_t)now;
}

time_source_t time_service_get_last_source(void) {
    return last_sync_source;
}

esp_err_t time_service_register_callback(time_sync_callback_t callback) {
    sync_callback = callback;
    return ESP_OK;
}

esp_err_t time_service_set_alarm(uint8_t hour, uint8_t minute, bool repeat_daily) {
    // TODO: Implement alarm using ESP-IDF alarm APIs
    ESP_LOGW(TAG, "Alarm not yet implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t time_service_cancel_alarm(void) {
    // TODO: Implement alarm cancellation
    return ESP_OK;
}

bool time_service_is_dst(void) {
    return config.is_dst;
}

esp_err_t time_service_set_dst(bool enable) {
    config.is_dst = enable;
    // TODO: Adjust timezone offset if DST is enabled
    return ESP_OK;
}
