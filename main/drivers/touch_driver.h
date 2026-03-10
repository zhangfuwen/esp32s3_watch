/**
 * @file touch_driver.h
 * @brief CST816 Touch Driver with OOP pattern
 */

#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// CST816 registers
#define CST816_CHIP_ID      0xA7
#define CST816_GEST_ID      0x01
#define CST816_FINGER_NUM   0x02
#define CST816_XPOSH        0x03
#define CST816_XPOSL        0x04
#define CST816_YPOSH        0x05
#define CST816_YPOSL        0x06

// Interface structure (vtable)
typedef struct {
    esp_err_t (*init)(void *self);
    esp_err_t (*deinit)(void *self);
    esp_err_t (*read_point)(void *self, uint16_t *x, uint16_t *y, bool *pressed);
    void (*lvgl_read_cb)(lv_indev_drv_t *drv, lv_indev_data_t *data);
} touch_driver_iface_t;

// Implementation structure
typedef struct {
    const touch_driver_iface_t *iface;
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    gpio_num_t int_pin;
    bool initialized;
    uint16_t last_x;
    uint16_t last_y;
    bool last_pressed;
} touch_driver_impl_t;

// Global instance
extern touch_driver_impl_t g_touch_driver;

// Public API
esp_err_t touch_driver_init(touch_driver_impl_t *driver, i2c_port_t i2c_port, 
                            uint8_t i2c_addr, gpio_num_t int_pin);
esp_err_t touch_driver_deinit(touch_driver_impl_t *driver);
esp_err_t touch_driver_read_point(touch_driver_impl_t *driver, 
                                   uint16_t *x, uint16_t *y, bool *pressed);
void touch_driver_register_lvgl(lv_indev_drv_t *indev_drv, touch_driver_impl_t *driver);

#ifdef __cplusplus
}
#endif

#endif // TOUCH_DRIVER_H
