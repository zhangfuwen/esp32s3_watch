#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "esp_err.h"

esp_err_t lvgl_display_init(void);
esp_err_t lvgl_init_system(void);
esp_err_t lvgl_start_tasks(void);

#endif
