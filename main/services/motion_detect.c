/**
 * @file motion_detect.c
 * @brief Wrist Wake Detection Implementation
 */

#include "motion_detect.h"
#include "imu_driver.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <inttypes.h>
#include <math.h>
#include <string.h>

// Forward declaration
extern void watch_face_chinese_user_activity(void);

static const char *TAG = "MOTION";

static struct {
    bool initialized;
    float last_acc_x;
    float last_acc_y;
    float last_acc_z;
    int64_t last_wake_time;
    uint32_t wake_count;
    int64_t last_update_time;
    float acc_velocity_x;  // Accumulated change
    float acc_velocity_y;
    float acc_velocity_z;
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
    
    int64_t now = esp_timer_get_time() / 1000;  // ms
    
    // Calculate delta (change in acceleration)
    float delta_x = data->x - s_motion.last_acc_x;
    float delta_y = data->y - s_motion.last_acc_y;
    float delta_z = data->z - s_motion.last_acc_z;
    
    // Calculate delta magnitude in mg
    float delta_mag = sqrtf(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z) * 1000.0f;
    
    // Accumulate velocity (simple integration)
    s_motion.acc_velocity_x += delta_x;
    s_motion.acc_velocity_y += delta_y;
    s_motion.acc_velocity_z += delta_z;
    
    // Decay velocity (damping)
    s_motion.acc_velocity_x *= 0.9f;
    s_motion.acc_velocity_y *= 0.9f;
    s_motion.acc_velocity_z *= 0.9f;
    
    // Store current values
    s_motion.last_acc_x = data->x;
    s_motion.last_acc_y = data->y;
    s_motion.last_acc_z = data->z;
    s_motion.last_update_time = now;
    
    // Log occasionally
    static uint32_t log_count = 0;
    log_count++;
    if (log_count % 500 == 0) {
        ESP_LOGD(TAG, "Acc: x=%.2f y=%.2f z=%.2f (g), delta=%.0f mg", 
                 data->x, data->y, data->z, delta_mag);
    }
}

bool motion_detect_check_wrist_wake(void) {
    if (!s_motion.initialized) {
        return false;
    }
    
    // Calculate velocity magnitude
    float velocity_mag = sqrtf(
        s_motion.acc_velocity_x * s_motion.acc_velocity_x +
        s_motion.acc_velocity_y * s_motion.acc_velocity_y +
        s_motion.acc_velocity_z * s_motion.acc_velocity_z
    );
    
    // Convert to mg
    float velocity_mg = velocity_mag * 1000.0f;
    
    // Check if exceeds threshold (lowered to 300mg for better sensitivity)
    if (velocity_mg > 300.0f) {
        int64_t now = esp_timer_get_time() / 1000;  // ms
        
        // Debounce: only trigger once per 2 seconds
        if (now - s_motion.last_wake_time > 2000) {
            s_motion.last_wake_time = now;
            s_motion.wake_count++;
            ESP_LOGI(TAG, "Wrist wake! vel=%.0f mg (count=%" PRIu32 ")", velocity_mg, s_motion.wake_count);
            
            // Turn on display
            #ifdef CONFIG_LVGL_TEST_ENABLED
            watch_face_chinese_user_activity();
            #endif
            
            return true;
        }
    }
    
    return false;
}
