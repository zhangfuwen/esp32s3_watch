/**
 * @file motion_detect.c
 * @brief Wrist Wake Detection Implementation
 */

#include "motion_detect.h"
#include "imu_driver.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>
#include <string.h>

static const char *TAG = "MOTION";

static struct {
    bool initialized;
    float last_acc_x;
    float last_acc_y;
    float last_acc_z;
    int64_t last_wake_time;
    uint32_t wake_count;
} s_motion = {0};

esp_err_t motion_detect_init(void) {
    ESP_LOGI(TAG, "Initializing motion detection...");
    
    memset(&s_motion, 0, sizeof(s_motion));
    s_motion.initialized = true;
    s_motion.last_wake_time = 0;
    s_motion.wake_count = 0;
    
    ESP_LOGI(TAG, "Motion detection initialized (threshold: %d mg)", MOTION_DETECT_THRESHOLD_MG);
    return ESP_OK;
}

esp_err_t motion_detect_deinit(void) {
    if (!s_motion.initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing motion detection");
    s_motion.initialized = false;
    return ESP_OK;
}

void motion_detect_update(imu_data_t *data) {
    if (!s_motion.initialized || !data) {
        return;
    }
    
    s_motion.last_acc_x = data->x;
    s_motion.last_acc_y = data->y;
    s_motion.last_acc_z = data->z;
}

bool motion_detect_check_wrist_wake(void) {
    if (!s_motion.initialized) {
        return false;
    }
    
    // Simple threshold-based detection
    // Calculate acceleration magnitude (data is already in g units)
    float acc_mag = sqrtf(
        s_motion.last_acc_x * s_motion.last_acc_x +
        s_motion.last_acc_y * s_motion.last_acc_y +
        s_motion.last_acc_z * s_motion.last_acc_z
    );
    
    // Convert to mg
    float acc_mg = acc_mag * 1000.0f;
    
    // Check if exceeds threshold
    if (acc_mg > MOTION_DETECT_THRESHOLD_MG) {
        int64_t now = esp_timer_get_time() / 1000;  // ms
        
        // Debounce: only trigger once per second
        if (now - s_motion.last_wake_time > 1000) {
            s_motion.last_wake_time = now;
            s_motion.wake_count++;
            ESP_LOGI(TAG, "Wrist wake detected! (acc=%.0f mg, count=%lu)", acc_mg, s_motion.wake_count);
            return true;
        }
    }
    
    return false;
}
