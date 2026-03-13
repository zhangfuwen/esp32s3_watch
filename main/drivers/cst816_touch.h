#ifndef CST816_TOUCH_H
#define CST816_TOUCH_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

esp_err_t cst816_touch_init(void);
esp_err_t cst816_touch_read(uint16_t *x, uint16_t *y, bool *pressed);

#endif
