#ifndef LVGL_TEST_H
#define LVGL_TEST_H

#include "esp_err.h"

esp_err_t lvgl_test_init(void);
esp_err_t lvgl_test_run(void);
void lvgl_test_user_activity(void);

#endif
