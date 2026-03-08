/**
 * Hardware Pin Definitions
 * 
 * @file hardware_pins.h
 * @brief ESP32-S3 Watch pin mapping per AGENTS.md
 */

#ifndef _HARDWARE_PINS_H_
#define _HARDWARE_PINS_H_

#include <driver/gpio.h>

//==================== Buttons ====================
#define PIN_BOOT                   GPIO_NUM_0
#define PIN_PWR                    GPIO_NUM_45

//==================== I2C Bus ====================
#define PIN_I2C_SDA                GPIO_NUM_1
#define PIN_I2C_SCL                GPIO_NUM_2

//==================== Display (ST7789V2) ====================
#define PIN_DISPLAY_CS             GPIO_NUM_12
#define PIN_DISPLAY_SCL            GPIO_NUM_5
#define PIN_DISPLAY_MOSI           GPIO_NUM_6
#define PIN_DISPLAY_DC             GPIO_NUM_11
#define PIN_DISPLAY_BACKLIGHT      GPIO_NUM_47

//==================== Audio (ES8311) ====================
#define PIN_I2S_MCLK               GPIO_NUM_21
#define PIN_I2S_WS                GPIO_NUM_14
#define PIN_I2S_BCLK              GPIO_NUM_18
#define PIN_I2S_DI                GPIO_NUM_17
#define PIN_I2S_DO                GPIO_NUM_13
#define PIN_PA_EN                  GPIO_NUM_48

//==================== IMU (QMI8658C) ====================
#define PIN_IMU_INT1               GPIO_NUM_41
#define PIN_IMU_INT2               GPIO_NUM_42

//==================== Touch (CST816) ====================
#define PIN_TOUCH_RST              GPIO_NUM_7
#define PIN_TOUCH_INT              GPIO_NUM_46

//==================== Other ====================
#define PIN_VIBRATOR               GPIO_NUM_4
#define PIN_SD_DAT0                GPIO_NUM_10
#define PIN_SD_CMD                 GPIO_NUM_8
#define PIN_SD_CLK                 GPIO_NUM_9
#define PIN_USB_DM                 GPIO_NUM_19
#define PIN_USB_DP                 GPIO_NUM_20

//==================== Display Parameters ====================
#define DISPLAY_WIDTH              240
#define DISPLAY_HEIGHT             284
#define DISPLAY_SWAP_XY            false
#define DISPLAY_MIRROR_X           false
#define DISPLAY_MIRROR_Y           false
#define DISPLAY_INVERT_COLOR       false

#endif // _HARDWARE_PINS_H_
