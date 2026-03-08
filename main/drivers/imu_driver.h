/**
 * @file imu_driver.h
 * @brief QMI8658C 6-Axis IMU Driver
 */

#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "board_config.h"

#define QMI8658C_I2C_ADDR         0x6B

#define QMI8658C_ID               0x05
#define QMI8658C_REVISION_ID      0x01

#define QMI8658C_REG_WHO_AM_I     0x00
#define QMI8658C_REG_REVISION     0x01
#define QMI8658C_REG_CTRL1        0x02
#define QMI8658C_REG_CTRL2        0x03
#define QMI8658C_REG_CTRL3        0x04
#define QMI8658C_REG_CTRL4        0x05
#define QMI8658C_REG_CTRL5        0x06
#define QMI8658C_REG_CTRL6        0x07
#define QMI8658C_REG_CTRL7        0x08
#define QMI8658C_REG_CTRL8        0x09
#define QMI8658C_REG_CTRL9        0x0A
#define QMI8658C_REG_CTRL10       0x0B

#define QMI8658C_REG_STATUS       0x2D
#define QMI8658C_REG_ACC_X_L      0x35
#define QMI8658C_REG_ACC_X_H      0x36
#define QMI8658C_REG_ACC_Y_L      0x37
#define QMI8658C_REG_ACC_Y_H      0x38
#define QMI8658C_REG_ACC_Z_L      0x39
#define QMI8658C_REG_ACC_Z_H      0x3A

#define QMI8658C_REG_GYR_X_L      0x3B
#define QMI8658C_REG_GYR_X_H      0x3C
#define QMI8658C_REG_GYR_Y_L      0x3D
#define QMI8658C_REG_GYR_Y_H      0x3E
#define QMI8658C_REG_GYR_Z_L      0x3F
#define QMI8658C_REG_GYR_Z_H      0x40

#define QMI8658C_REG_TEMP_L       0x41
#define QMI8658C_REG_TEMP_H       0x42

#define QMI8658C_REG_TIMESTAMP_L  0x43
#define QMI8658C_REG_TIMESTAMP_M  0x44
#define QMI8658C_REG_TIMESTAMP_H  0x45

#define QMI8658C_REG_INT1         0x4C
#define QMI8658C_REG_INT2         0x4D
#define QMI8658C_REG_INT_MSK1     0x4E
#define QMI8658C_REG_INT_MSK2     0x4F

#define QMI8658C_ACC_RANGE_2G     0x00
#define QMI8658C_ACC_RANGE_4G     0x20
#define QMI8658C_ACC_RANGE_8G     0x40
#define QMI8658C_ACC_RANGE_16G    0x60

#define QMI8658C_GYR_RANGE_16DPS  0x00
#define QMI8658C_GYR_RANGE_32DPS  0x08
#define QMI8658C_GYR_RANGE_64DPS  0x10
#define QMI8658C_GYR_RANGE_128DPS 0x18
#define QMI8658C_GYR_RANGE_256DPS 0x20
#define QMI8658C_GYR_RANGE_512DPS 0x28
#define QMI8658C_GYR_RANGE_1024DPS 0x30
#define QMI8658C_GYR_RANGE_2048DPS 0x38

#define QMI8658C_ACC_ODR_7_5HZ    0x01
#define QMI8658C_ACC_ODR_15HZ     0x02
#define QMI8658C_ACC_ODR_30HZ    0x03
#define QMI8658C_ACC_ODR_60HZ    0x04
#define QMI8658C_ACC_ODR_120HZ   0x05
#define QMI8658C_ACC_ODR_240HZ   0x06
#define QMI8658C_ACC_ODR_480HZ   0x07
#define QMI8658C_ACC_ODR_960HZ   0x08
#define QMI8658C_ACC_ODR_1920HZ  0x09
#define QMI8658C_ACC_ODR_3840HZ  0x0A
#define QMI8658C_ACC_ODR_7_5HZ_LP 0x11
#define QMI8658C_ACC_ODR_15HZ_LP  0x12

#define QMI8658C_GYR_ODR_7_5HZ    0x01
#define QMI8658C_GYR_ODR_15HZ    0x02
#define QMI8658C_GYR_ODR_30HZ    0x03
#define QMI8658C_GYR_ODR_60HZ    0x04
#define QMI8658C_GYR_ODR_120HZ   0x05
#define QMI8658C_GYR_ODR_240HZ   0x06
#define QMI8658C_GYR_ODR_480HZ   0x07
#define QMI8658C_GYR_ODR_960HZ   0x08
#define QMI8658C_GYR_ODR_1920HZ  0x09
#define QMI8658C_GYR_ODR_3840HZ  0x0A

typedef enum {
    IMU_ACC_RANGE_2G,
    IMU_ACC_RANGE_4G,
    IMU_ACC_RANGE_8G,
    IMU_ACC_RANGE_16G,
} imu_acc_range_t;

typedef enum {
    IMU_GYR_RANGE_16DPS,
    IMU_GYR_RANGE_32DPS,
    IMU_GYR_RANGE_64DPS,
    IMU_GYR_RANGE_128DPS,
    IMU_GYR_RANGE_256DPS,
    IMU_GYR_RANGE_512DPS,
    IMU_GYR_RANGE_1024DPS,
    IMU_GYR_RANGE_2048DPS,
} imu_gyr_range_t;

typedef enum {
    IMU_ACC_ODR_7_5HZ,
    IMU_ACC_ODR_15HZ,
    IMU_ACC_ODR_30HZ,
    IMU_ACC_ODR_60HZ,
    IMU_ACC_ODR_120HZ,
    IMU_ACC_ODR_240HZ,
    IMU_ACC_ODR_480HZ,
    IMU_ACC_ODR_960HZ,
    IMU_ACC_ODR_1920HZ,
    IMU_ACC_ODR_3840HZ,
} imu_acc_odr_t;

typedef enum {
    IMU_GYR_ODR_7_5HZ,
    IMU_GYR_ODR_15HZ,
    IMU_GYR_ODR_30HZ,
    IMU_GYR_ODR_60HZ,
    IMU_GYR_ODR_120HZ,
    IMU_GYR_ODR_240HZ,
    IMU_GYR_ODR_480HZ,
    IMU_GYR_ODR_960HZ,
    IMU_GYR_ODR_1920HZ,
    IMU_GYR_ODR_3840HZ,
} imu_gyr_odr_t;

typedef struct {
    float x;
    float y;
    float z;
} imu_acc_data_t;

typedef struct {
    float x;
    float y;
    float z;
} imu_gyr_data_t;

typedef struct {
    float x;
    float y;
    float z;
    float temperature;
} imu_data_t;

typedef struct {
    imu_acc_range_t acc_range;
    imu_gyr_range_t gyr_range;
    imu_acc_odr_t acc_odr;
    imu_gyr_odr_t gyr_odr;
    bool enable_acc;
    bool enable_gyr;
    bool enable_wrist_wakeup;
    bool enable_tap;
    bool enable_any_motion;
} imu_config_t;

typedef void (*imu_data_callback_t)(const imu_data_t *data);

typedef void (*imu_wrist_wakeup_callback_t)(void);

esp_err_t imu_driver_init(const imu_config_t *config);

esp_err_t imu_driver_deinit(void);

esp_err_t imu_driver_read_acc(imu_acc_data_t *acc);

esp_err_t imu_driver_read_gyr(imu_gyr_data_t *gyr);

esp_err_t imu_driver_read_all(imu_data_t *data);

esp_err_t imu_driver_enable_wrist_wakeup(imu_wrist_wakeup_callback_t callback);

esp_err_t imu_driver_disable_wrist_wakeup(void);

esp_err_t imu_driver_set_low_power_mode(bool enable);

esp_err_t imu_driver_calibrate(void);

bool imu_driver_is_available(void);

uint8_t imu_driver_get_device_id(void);

#endif // IMU_DRIVER_H
