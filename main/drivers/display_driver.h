/**
 * @file display_driver.h
 * @brief ST7789 LCD Display Driver
 */

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "board_config.h"

#define DISPLAY_DRIVER_TAG "ST7789"

#define ST7789_WIDTH        240
#define ST7789_HEIGHT       284
#define ST7789_HEIGHT_CFG   240

#define ST7789_COLOR_MODE_65K  0x50
#define ST7789_COLOR_MODE_262K 0x60
#define ST7789_COLOR_MODE_12BIT 0x03
#define ST7789_COLOR_MODE_16BIT 0x55
#define ST7789_COLOR_MODE_18BIT 0x06

#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00
#define ST7789_MADCTL_BGR 0x08
#define ST7789_MADCTL_GS  0x01
#define ST7789_MADCTL_SS  0x02

#define ST7789_NOP         0x00
#define ST7789_SWRESET     0x01
#define ST7789_RDDID       0x04
#define ST7789_RDNUMAX     0x0A
#define ST7789_RDDPM       0x0B
#define ST7789_RDDMADCTL   0x0B
#define ST7789_RDDCOLMOD   0x0C
#define ST7789_RDDCIM      0x0D
#define ST7789_RDST        0x0F
#define ST7789_RDDPM       0x0B
#define ST7789_SLPIN       0x10
#define ST7789_SLPOUT      0x11
#define ST7789_PTLON       0x12
#define ST7789_NORON       0x13
#define ST7789_INVOFF      0x20
#define ST7789_INVON       0x21
#define ST7789_GAMMA       0x26
#define ST7789_DISPOFF     0x28
#define ST7789_DISPON      0x29
#define ST7789_CASET       0x2A
#define ST7789_RASET       0x2B
#define ST7789_RAMWR       0x2C
#define ST7789_RAMRD       0x2E
#define ST7789_PTLAR       0x30
#define ST7789_MADCTL      0x36
#define ST7789_VSCRDEF     0x37
#define ST7789_COLMOD      0x3A
#define ST7789_VCOMS       0xB4
#define ST7789_LCMCTRL     0xC0
#define ST7789_IDSET       0xC1
#define ST7789_VDVVRHEN    0xC2
#define ST7789_VRHS        0xC3
#define ST7789_VDVS        0xC4
#define ST7789_VCOMW       0xC5
#define ST7789_FRCTRL2     0xC6
#define ST7789_PWCTRL1     0xD0
#define ST7789_RDID1       0xDA
#define ST7789_RDID2       0xDB
#define ST7789_RDID3       0xDC
#define ST7789_PVGAMCTRL   0xE0
#define ST7789_NVGAMCTRL   0xE1
#define ST7789_DGAMCTRL1   0xE2
#define ST7789_DGAMCTRL2   0xE3
#define ST7789_PWCTR6      0xFC

typedef enum {
    DISPLAY_ROTATION_0,
    DISPLAY_ROTATION_90,
    DISPLAY_ROTATION_180,
    DISPLAY_ROTATION_270,
} display_rotation_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
} display_window_t;

typedef struct {
    uint8_t spi_host;
    int8_t pin_mosi;
    int8_t pin_sclk;
    int8_t pin_cs;
    int8_t pin_dc;
    int8_t pin_rst;
    int8_t pin_backlight;
    uint32_t spi_speed;
    display_rotation_t rotation;
    bool invert_colors;
    bool swap_xy;
} display_driver_config_t;

esp_err_t display_driver_init(const display_driver_config_t *config);

esp_err_t display_driver_deinit(void);

esp_err_t display_driver_set_rotation(display_rotation_t rotation);

esp_err_t display_driver_set_window(const display_window_t *window);

esp_err_t display_driver_write_pixels(const uint16_t *pixels, size_t count);

esp_err_t display_driver_fill_rect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color);

esp_err_t display_driver_fill_screen(uint16_t color);

esp_err_t display_driver_set_pixel(int16_t x, int16_t y, uint16_t color);

esp_err_t display_driver_sleep(void);

esp_err_t display_driver_wake(void);

esp_err_t display_driver_set_backlight(uint8_t brightness);

uint16_t display_driver_get_width(void);

uint16_t display_driver_get_height(void);

#endif // DISPLAY_DRIVER_H
