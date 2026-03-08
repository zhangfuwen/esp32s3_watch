/**
 * System Configuration
 * 
 * @file system_config.h
 * @brief ESP32-S3 Watch system settings
 */

#ifndef _SYSTEM_CONFIG_H_
#define _SYSTEM_CONFIG_H_

//==================== Project Info ====================
#define PROJECT_NAME               "ESP32-Watch"
#define PROJECT_VERSION            "0.1.0"
#define HARDWARE_VERSION           "ESPWatch-S3B2"

//==================== Core Settings ====================
#define CONFIG_ESP32S3_CORE_NUMBER 2
#define CONFIG_ESP32S3_CPU_FREQ_240

//==================== Display Settings ====================
#define DISPLAY_TASK_STACK_SIZE    4096
#define DISPLAY_TASK_PRIORITY      10
#define DISPLAY_REFRESH_RATE_HZ    60

//==================== UI Settings ====================
#define UI_TASK_STACK_SIZE         4096
#define UI_TASK_PRIORITY           5

//==================== Power Management ====================
#define POWER_MANAGEMENT_ENABLED   1
#define LIGHT_SLEEP_TIMEOUT_MS     30000
#define DEEP_SLEEP_TIMEOUT_MS      300000

//==================== I2C Settings ====================
#define I2C_MASTER_NUM             I2C_NUM_0
#define I2C_MASTER_FREQ_HZ         400000
#define I2C_TIMEOUT_MS             1000

//==================== SPI Settings ====================
#define SPI_DISPLAY_HOST           SPI2_HOST
#define SPI_DISPLAY_FREQ_HZ        40000000
#define SPI_DMA_CH                 SPI_DMA_CH_AUTO

//==================== Audio Settings ====================
#define AUDIO_TASK_STACK_SIZE      4096
#define AUDIO_SAMPLE_RATE          24000
#define AUDIO_BUFFER_SIZE          512

//==================== IMU Settings ====================
#define IMU_TASK_STACK_SIZE        2048
#define IMU_SAMPLE_RATE_HZ         100

//==================== Storage ====================
#define SPIFFS_PARTITION_LABEL     "storage"
#define SPIFFS_MAX_FILES           10

//==================== Logging ====================
#define LOG_LOCAL_LEVEL            ESP_LOG_INFO

#endif // _SYSTEM_CONFIG_H_
