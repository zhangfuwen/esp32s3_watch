/**
 * @file time_update.h
 * @brief Periodic Time Update Service
 */

#ifndef TIME_UPDATE_H
#define TIME_UPDATE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t time_update_start(void);
esp_err_t time_update_stop(void);

#ifdef __cplusplus
}
#endif

#endif // TIME_UPDATE_H
