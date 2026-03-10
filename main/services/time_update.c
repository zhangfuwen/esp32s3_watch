/**
 * @file time_update.c
 * @brief Periodic Time Update Service
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
// #include "watch_face_ui.h"  // Temporarily disabled
#include "battery_driver.h"
#include <time.h>
#include <stdio.h>

static const char *TAG = "TIME_UPDATE";

static struct {
    TaskHandle_t task_handle;
    bool running;
    uint8_t last_hour;
    uint8_t last_minute;
    uint8_t last_second;
} s_time_update = {0};

static void time_update_task(void *pvParameters) {
    ESP_LOGI(TAG, "Time update task started");
    
    while (s_time_update.running) {
        // Get current time
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        
        uint8_t hour = timeinfo.tm_hour;
        uint8_t minute = timeinfo.tm_min;
        uint8_t second = timeinfo.tm_sec;
        
        // Update time display (DISABLED FOR NOW)
        // watch_face_ui_update_time(hour, minute, second);
        
        // Update date every minute
        if (second == 0 && minute != s_time_update.last_minute) {
            // watch_face_ui_update_date(
            //     timeinfo.tm_year + 1900,
            //     timeinfo.tm_mon + 1,
            //     timeinfo.tm_mday
            // );
            
            // Update battery every minute
            uint8_t soc = 0;
            uint16_t voltage = 0;
            battery_driver_get_soc(&g_battery_driver, &soc);
            battery_driver_get_voltage(&g_battery_driver, &voltage);
            // watch_face_ui_update_battery(soc, voltage);
            
            ESP_LOGI(TAG, "Time: %02d:%02d:%02d, Battery: %d%% (%d mV)",
                     hour, minute, second, soc, voltage);
        }
        
        s_time_update.last_hour = hour;
        s_time_update.last_minute = minute;
        s_time_update.last_second = second;
        
        // Update every second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    vTaskDelete(NULL);
}

esp_err_t time_update_start(void) {
    if (s_time_update.running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting time update service...");
    
    s_time_update.running = true;
    xTaskCreate(time_update_task, "time_update", 4096, NULL, 5, &s_time_update.task_handle);
    
    return ESP_OK;
}

esp_err_t time_update_stop(void) {
    if (!s_time_update.running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Stopping time update service");
    s_time_update.running = false;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    return ESP_OK;
}
