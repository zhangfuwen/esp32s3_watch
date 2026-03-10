/**
 * @file imu_task.c
 * @brief IMU Polling Task for Wrist Wake
 */

#include "imu_task.h"
#include "imu_driver.h"
#include "motion_detect.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Forward declaration
extern void watch_face_user_activity(void);

static const char *TAG = "IMU_TASK";

static TaskHandle_t s_imu_task_handle = NULL;
static bool s_imu_available = false;

// IMU polling task
static void imu_poll_task(void *pvParameters) {
    ESP_LOGI(TAG, "IMU poll task started");
    
    // Initialize IMU with default config (use valid constants)
    const imu_config_t config = {
        .acc_range = IMU_ACC_RANGE_2G,
        .gyr_range = IMU_GYR_RANGE_256DPS,
        .acc_odr = IMU_ACC_ODR_120HZ,
        .gyr_odr = IMU_GYR_ODR_120HZ,
        .enable_acc = true,
        .enable_gyr = true,
        .enable_tap = false,
    };
    
    esp_err_t ret = imu_driver_init(&config);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "IMU init failed: %s", esp_err_to_name(ret));
        s_imu_available = false;
    } else {
        ESP_LOGI(TAG, "IMU initialized");
        s_imu_available = true;
    }
    
    imu_data_t imu_data = {0};
    uint32_t read_count = 0;
    uint32_t wake_count = 0;
    
    while (1) {
        if (s_imu_available) {
            // Read IMU data
            ret = imu_driver_read_all(&imu_data);
            
            if (ret == ESP_OK) {
                read_count++;
                
                // Update motion detector
                motion_detect_update(&imu_data);
                
                // Check for wrist wake
                if (motion_detect_check_wrist_wake()) {
                    wake_count++;
                    ESP_LOGI(TAG, "Wrist wake! (count=%" PRIu32 ", reads=%" PRIu32 ")", wake_count, read_count);
                    // Trigger display wake
                    watch_face_user_activity();
                }
            }
        }
        
        // Poll at 50Hz (20ms)
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t imu_task_start(void) {
    ESP_LOGI(TAG, "Starting IMU task...");
    
    // Create polling task
    xTaskCreate(imu_poll_task, "imu_poll", 4096, NULL, 5, &s_imu_task_handle);
    
    ESP_LOGI(TAG, "IMU task created");
    
    return ESP_OK;
}

esp_err_t imu_task_stop(void) {
    if (s_imu_task_handle) {
        vTaskDelete(s_imu_task_handle);
        s_imu_task_handle = NULL;
    }
    
    imu_driver_deinit();
    
    ESP_LOGI(TAG, "IMU task stopped");
    
    return ESP_OK;
}

bool imu_task_is_running(void) {
    return (s_imu_task_handle != NULL);
}
