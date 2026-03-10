/**
 * ESP32-S3 Watch Board Configuration
 * 
 * @file board_config.h
 * @brief Hardware pin definitions and board-specific configuration
 * @version 0.1
 * @date 2026-03-06
 * 
 * Board: espwatch-s3b2
 */

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

//==================== Audio Configuration ====================
#define AUDIO_INPUT_SAMPLE_RATE     24000
#define AUDIO_OUTPUT_SAMPLE_RATE    24000
#define AUDIO_INPUT_REFERENCE       true

// I2S Audio Pins
#define AUDIO_I2S_GPIO_MCLK         GPIO_NUM_21
#define AUDIO_I2S_GPIO_WS           GPIO_NUM_14
#define AUDIO_I2S_GPIO_BCLK         GPIO_NUM_18
#define AUDIO_I2S_GPIO_DIN          GPIO_NUM_17
#define AUDIO_I2S_GPIO_DOUT         GPIO_NUM_13

// Audio Codec (ES8311 with PCA9557)
#define AUDIO_CODEC_USE_PCA9557
#define AUDIO_CODEC_PA_PIN          GPIO_NUM_48
#define AUDIO_CODEC_I2C_SDA_PIN     GPIO_NUM_1
#define AUDIO_CODEC_I2C_SCL_PIN     GPIO_NUM_2
#define AUDIO_CODEC_ES8311_ADDR     ES8311_CODEC_DEFAULT_ADDR

//==================== Buttons & Input ====================
#define BUILTIN_LED_GPIO            GPIO_NUM_NC       // Not connected
#define BOOT_BUTTON_GPIO            GPIO_NUM_0        // Boot button
#define POWER_BUTTON_GPIO           GPIO_NUM_45       // Power button
#define VOLUME_UP_BUTTON_GPIO       GPIO_NUM_NC       // Not connected
#define VOLUME_DOWN_BUTTON_GPIO     GPIO_NUM_NC       // Not connected

//==================== Display Configuration ====================
// Display uses I2C shared with audio codec
#define DISPLAY_SDA_PIN             AUDIO_CODEC_I2C_SDA_PIN   // GPIO 1
#define DISPLAY_SCL_PIN             AUDIO_CODEC_I2C_SCL_PIN   // GPIO 2

// Display SPI pins (for LCD)
// #define LCD_TYPE_JD9853_SERIAL    // Uncomment if using JD9853 serial LCD
#define DISPLAY_MISO_PIN            GPIO_NUM_NC       // Not connected
#define DISPLAY_MOSI_PIN            GPIO_NUM_6
#define DISPLAY_SCLK_PIN            GPIO_NUM_5
#define DISPLAY_CS_PIN              GPIO_NUM_12
#define DISPLAY_DC_PIN              GPIO_NUM_11
#define DISPLAY_RESET_PIN           GPIO_NUM_NC       // Not connected
#define DISPLAY_BACKLIGHT_PIN       GPIO_NUM_47

// Vibration Motor
#define VIB_MOTOR_PIN               GPIO_NUM_4

//==================== Display Parameters ====================
#define DISPLAY_WIDTH               240
#define DISPLAY_HEIGHT              284

// Display orientation and color settings
#define DISPLAY_SWAP_XY             false
#define DISPLAY_MIRROR_X            false
#define DISPLAY_MIRROR_Y            false
#define DISPLAY_INVERT_COLOR        false
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

// Display offset
#define DISPLAY_OFFSET_X            0
#define DISPLAY_OFFSET_Y            0

//==================== I2C Configuration ====================
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TX_BUF_DISABLE   0

// I2C GPIO pins (shared with audio codec)
#define I2C_MASTER_SDA_IO           AUDIO_CODEC_I2C_SDA_PIN  // GPIO 1
#define I2C_MASTER_SCL_IO           AUDIO_CODEC_I2C_SCL_PIN  // GPIO 2

//==================== Power Management ====================
// TODO: Add ADC pin for battery voltage monitoring
// TODO: Add charging status pin

//==================== Battery Fuel Gauge (MAX17048G) ====================
#define BATTERY_GAUGE_I2C_ADDR      0x36  // MAX17048G default address

//==================== 6-Axis IMU (QMI8658C) ====================
#define IMU_I2C_ADDR                0x6B  // QMI8658C default address
#define IMU_INT1_GPIO               GPIO_NUM_41
#define IMU_INT2_GPIO               GPIO_NUM_42

//==================== Touch Screen (CST816) ====================
#define TOUCH_I2C_ADDR              0x15  // CST816 default address
#define TOUCH_RESET_GPIO            GPIO_NUM_7
#define TOUCH_INT_GPIO              GPIO_NUM_46

//==================== Heart Rate & SpO2 (MAX30102) ====================
#define HEART_RATE_I2C_ADDR         0x57  // MAX30102 default address

//==================== SD Card (SPI/SDIO) ====================
#define SD_CARD_MISO_PIN            GPIO_NUM_8
#define SD_CARD_MOSI_PIN            GPIO_NUM_9
#define SD_CARD_SCLK_PIN            GPIO_NUM_10
#define SD_CARD_CS_PIN              GPIO_NUM_NC  // Not used in 1-bit SDIO mode

#endif // _BOARD_CONFIG_H_
