/**
 * @file motion_detect.h
 * @brief Wrist Wake Detection Service
 */

#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "imu_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configuration
#define MOTION_DETECT_THRESHOLD_MG  800   // mg for wrist raise
#define MOTION_DETECT_WINDOW_MS     500   // Detection window

// Public API
esp_err_t motion_detect_init(void);
esp_err_t motion_detect_deinit(void);
bool motion_detect_check_wrist_wake(void);
void motion_detect_update(imu_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // MOTION_DETECT_H
