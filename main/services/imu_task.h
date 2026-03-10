#ifndef IMU_TASK_H
#define IMU_TASK_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t imu_task_start(void);
esp_err_t imu_task_stop(void);
bool imu_task_is_running(void);

#endif
