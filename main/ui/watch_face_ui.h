/**
 * @file watch_face_ui.h
 * @brief Watch Face UI with LVGL
 */

#ifndef WATCH_FACE_UI_H
#define WATCH_FACE_UI_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Public API
void watch_face_ui_init(void);
void watch_face_ui_update_time(uint8_t hour, uint8_t minute, uint8_t second);
void watch_face_ui_update_date(uint16_t year, uint8_t month, uint8_t day);
void watch_face_ui_update_battery(uint8_t percent, uint16_t voltage_mv);
lv_obj_t* watch_face_ui_get_screen(void);

#ifdef __cplusplus
}
#endif

#endif // WATCH_FACE_UI_H
