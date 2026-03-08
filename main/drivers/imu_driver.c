/**
 * @file imu_driver.c
 * @brief QMI8658C 6-Axis IMU Driver Implementation
 */

#include "imu_driver.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

static const char *TAG = "QMI8658C";

static bool s_initialized = false;
static i2c_port_t s_i2c_port = I2C_NUM_0;
static imu_config_t s_config;
static imu_wrist_wakeup_callback_t s_wrist_wakeup_callback = NULL;
static bool s_low_power_enabled = false;

static const float s_acc_scale[] = { 2.0f / 32768.0f, 4.0f / 32768.0f, 8.0f / 32768.0f, 16.0f / 32768.0f };
static const float s_gyr_scale[] = { 16.0f / 32768.0f, 32.0f / 32768.0f, 64.0f / 32768.0f, 128.0f / 32768.0f,
                                      256.0f / 32768.0f, 512.0f / 32768.0f, 1024.0f / 32768.0f, 2048.0f / 32768.0f };

static esp_err_t imu_driver_write_register(uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t imu_driver_read_register(uint8_t reg, uint8_t *value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, value, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t imu_driver_read_registers(uint8_t reg, uint8_t *buffer, size_t length)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &buffer[length - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return ret;
}

static uint8_t acc_range_to_reg(imu_acc_range_t range)
{
    switch (range) {
        case IMU_ACC_RANGE_2G:  return QMI8658C_ACC_RANGE_2G;
        case IMU_ACC_RANGE_4G:  return QMI8658C_ACC_RANGE_4G;
        case IMU_ACC_RANGE_8G:  return QMI8658C_ACC_RANGE_8G;
        case IMU_ACC_RANGE_16G: return QMI8658C_ACC_RANGE_16G;
        default:                return QMI8658C_ACC_RANGE_2G;
    }
}

static uint8_t gyr_range_to_reg(imu_gyr_range_t range)
{
    switch (range) {
        case IMU_GYR_RANGE_16DPS:   return QMI8658C_GYR_RANGE_16DPS;
        case IMU_GYR_RANGE_32DPS:   return QMI8658C_GYR_RANGE_32DPS;
        case IMU_GYR_RANGE_64DPS:   return QMI8658C_GYR_RANGE_64DPS;
        case IMU_GYR_RANGE_128DPS:  return QMI8658C_GYR_RANGE_128DPS;
        case IMU_GYR_RANGE_256DPS:  return QMI8658C_GYR_RANGE_256DPS;
        case IMU_GYR_RANGE_512DPS:  return QMI8658C_GYR_RANGE_512DPS;
        case IMU_GYR_RANGE_1024DPS: return QMI8658C_GYR_RANGE_1024DPS;
        case IMU_GYR_RANGE_2048DPS: return QMI8658C_GYR_RANGE_2048DPS;
        default:                    return QMI8658C_GYR_RANGE_256DPS;
    }
}

static uint8_t acc_odr_to_reg(imu_acc_odr_t odr)
{
    switch (odr) {
        case IMU_ACC_ODR_7_5HZ:   return QMI8658C_ACC_ODR_7_5HZ;
        case IMU_ACC_ODR_15HZ:    return QMI8658C_ACC_ODR_15HZ;
        case IMU_ACC_ODR_30HZ:    return QMI8658C_ACC_ODR_30HZ;
        case IMU_ACC_ODR_60HZ:    return QMI8658C_ACC_ODR_60HZ;
        case IMU_ACC_ODR_120HZ:   return QMI8658C_ACC_ODR_120HZ;
        case IMU_ACC_ODR_240HZ:   return QMI8658C_ACC_ODR_240HZ;
        case IMU_ACC_ODR_480HZ:   return QMI8658C_ACC_ODR_480HZ;
        case IMU_ACC_ODR_960HZ:   return QMI8658C_ACC_ODR_960HZ;
        case IMU_ACC_ODR_1920HZ:  return QMI8658C_ACC_ODR_1920HZ;
        case IMU_ACC_ODR_3840HZ:  return QMI8658C_ACC_ODR_3840HZ;
        default:                  return QMI8658C_ACC_ODR_120HZ;
    }
}

static uint8_t gyr_odr_to_reg(imu_gyr_odr_t odr)
{
    switch (odr) {
        case IMU_GYR_ODR_7_5HZ:   return QMI8658C_GYR_ODR_7_5HZ;
        case IMU_GYR_ODR_15HZ:    return QMI8658C_GYR_ODR_15HZ;
        case IMU_GYR_ODR_30HZ:    return QMI8658C_GYR_ODR_30HZ;
        case IMU_GYR_ODR_60HZ:    return QMI8658C_GYR_ODR_60HZ;
        case IMU_GYR_ODR_120HZ:   return QMI8658C_GYR_ODR_120HZ;
        case IMU_GYR_ODR_240HZ:   return QMI8658C_GYR_ODR_240HZ;
        case IMU_GYR_ODR_480HZ:   return QMI8658C_GYR_ODR_480HZ;
        case IMU_GYR_ODR_960HZ:   return QMI8658C_GYR_ODR_960HZ;
        case IMU_GYR_ODR_1920HZ:  return QMI8658C_GYR_ODR_1920HZ;
        case IMU_GYR_ODR_3840HZ:  return QMI8658C_GYR_ODR_3840HZ;
        default:                  return QMI8658C_GYR_ODR_120HZ;
    }
}

static void imu_driver_isr_handler(void *arg)
{
    (void)arg;
    if (s_wrist_wakeup_callback) {
        s_wrist_wakeup_callback();
    }
}

esp_err_t imu_driver_init(const imu_config_t *config)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing QMI8658C IMU");

    if (config) {
        s_config = *config;
    } else {
        s_config.acc_range = IMU_ACC_RANGE_4G;
        s_config.gyr_range = IMU_GYR_RANGE_256DPS;
        s_config.acc_odr = IMU_ACC_ODR_120HZ;
        s_config.gyr_odr = IMU_GYR_ODR_120HZ;
        s_config.enable_acc = true;
        s_config.enable_gyr = false;
        s_config.enable_wrist_wakeup = false;
        s_config.enable_tap = false;
        s_config.enable_any_motion = false;
    }

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = DISPLAY_SDA_PIN,
        .scl_io_num = DISPLAY_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(s_i2c_port, &i2c_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(s_i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    uint8_t dev_id = 0;
    ret = imu_driver_read_register(QMI8658C_REG_WHO_AM_I, &dev_id);
    if (ret != ESP_OK || dev_id != QMI8658C_ID) {
        ESP_LOGE(TAG, "Device not found: 0x%02X (expected 0x%02X)", dev_id, QMI8658C_ID);
        i2c_driver_install(s_i2c_port, I2C_MODE_MASTER, 0, 0, 0);
        return ESP_ERR_NOT_FOUND;
    }

    imu_driver_write_register(QMI8658C_REG_CTRL1, 0x40);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t ctrl1_val = 0;
    if (s_config.enable_gyr) {
        ctrl1_val |= 0x80;
    }
    if (s_config.enable_acc) {
        ctrl1_val |= 0x40;
    }
    imu_driver_write_register(QMI8658C_REG_CTRL1, ctrl1_val);

    imu_driver_write_register(QMI8658C_REG_CTRL2, acc_range_to_reg(s_config.acc_range) | gyr_range_to_reg(s_config.gyr_range));

    imu_driver_write_register(QMI8658C_REG_CTRL3, acc_odr_to_reg(s_config.acc_odr) | (gyr_odr_to_reg(s_config.gyr_odr) << 4));

    if (s_config.enable_wrist_wakeup) {
        imu_driver_write_register(QMI8658C_REG_CTRL7, 0x00);
        imu_driver_write_register(QMI8658C_REG_CTRL8, 0x00);
    }

    gpio_config_t int_conf = {
        .pin_bit_mask = (1ULL << IMU_INT1_GPIO) | (1ULL << IMU_INT2_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&int_conf);

    s_initialized = true;
    ESP_LOGI(TAG, "QMI8658C initialized successfully");

    return ESP_OK;
}

esp_err_t imu_driver_deinit(void)
{
    if (!s_initialized) {
        return ESP_OK;
    }

    imu_driver_write_register(QMI8658C_REG_CTRL1, 0x00);

    gpio_reset_pin(IMU_INT1_GPIO);
    gpio_reset_pin(IMU_INT2_GPIO);

    i2c_driver_delete(s_i2c_port);
    s_initialized = false;

    return ESP_OK;
}

esp_err_t imu_driver_read_acc(imu_acc_data_t *acc)
{
    if (!s_initialized || !acc) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buffer[6];
    esp_err_t ret = imu_driver_read_registers(QMI8658C_REG_ACC_X_L, buffer, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw_x = (int16_t)(buffer[1] << 8 | buffer[0]);
    int16_t raw_y = (int16_t)(buffer[3] << 8 | buffer[2]);
    int16_t raw_z = (int16_t)(buffer[5] << 8 | buffer[4]);

    acc->x = raw_x * s_acc_scale[s_config.acc_range];
    acc->y = raw_y * s_acc_scale[s_config.acc_range];
    acc->z = raw_z * s_acc_scale[s_config.acc_range];

    return ESP_OK;
}

esp_err_t imu_driver_read_gyr(imu_gyr_data_t *gyr)
{
    if (!s_initialized || !gyr) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buffer[6];
    esp_err_t ret = imu_driver_read_registers(QMI8658C_REG_GYR_X_L, buffer, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw_x = (int16_t)(buffer[1] << 8 | buffer[0]);
    int16_t raw_y = (int16_t)(buffer[3] << 8 | buffer[2]);
    int16_t raw_z = (int16_t)(buffer[5] << 8 | buffer[4]);

    gyr->x = raw_x * s_gyr_scale[s_config.gyr_range];
    gyr->y = raw_y * s_gyr_scale[s_config.gyr_range];
    gyr->z = raw_z * s_gyr_scale[s_config.gyr_range];

    return ESP_OK;
}

esp_err_t imu_driver_read_all(imu_data_t *data)
{
    if (!s_initialized || !data) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buffer[12];
    esp_err_t ret = imu_driver_read_registers(QMI8658C_REG_ACC_X_L, buffer, 12);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw_acc_x = (int16_t)(buffer[1] << 8 | buffer[0]);
    int16_t raw_acc_y = (int16_t)(buffer[3] << 8 | buffer[2]);
    int16_t raw_acc_z = (int16_t)(buffer[5] << 8 | buffer[4]);

    int16_t raw_gyr_x = (int16_t)(buffer[7] << 8 | buffer[6]);
    int16_t raw_gyr_y = (int16_t)(buffer[9] << 8 | buffer[8]);
    int16_t raw_gyr_z = (int16_t)(buffer[11] << 8 | buffer[10]);

    data->x = raw_acc_x * s_acc_scale[s_config.acc_range];
    data->y = raw_acc_y * s_acc_scale[s_config.acc_range];
    data->z = raw_acc_z * s_acc_scale[s_config.acc_range];
    data->temperature = 0.0f;

    return ESP_OK;
}

esp_err_t imu_driver_enable_wrist_wakeup(imu_wrist_wakeup_callback_t callback)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_wrist_wakeup_callback = callback;

    imu_driver_write_register(QMI8658C_REG_CTRL7, 0x00);
    imu_driver_write_register(QMI8658C_REG_CTRL8, 0x10);

    imu_driver_write_register(QMI8658C_REG_INT1, 0x01);
    imu_driver_write_register(QMI8658C_REG_INT_MSK1, 0x00);

    gpio_isr_handler_add(IMU_INT1_GPIO, imu_driver_isr_handler, NULL);

    return ESP_OK;
}

esp_err_t imu_driver_disable_wrist_wakeup(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_wrist_wakeup_callback = NULL;
    gpio_isr_handler_remove(IMU_INT1_GPIO);

    imu_driver_write_register(QMI8658C_REG_INT_MSK1, 0xFF);

    return ESP_OK;
}

esp_err_t imu_driver_set_low_power_mode(bool enable)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_low_power_enabled = enable;

    if (enable) {
        imu_driver_write_register(QMI8658C_REG_CTRL3, QMI8658C_ACC_ODR_7_5HZ_LP);
    } else {
        imu_driver_write_register(QMI8658C_REG_CTRL3, acc_odr_to_reg(s_config.acc_odr));
    }

    return ESP_OK;
}

esp_err_t imu_driver_calibrate(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    imu_acc_data_t acc;
    float acc_x_sum = 0, acc_y_sum = 0, acc_z_sum = 0;

    for (int i = 0; i < 100; i++) {
        imu_driver_read_acc(&acc);
        acc_x_sum += acc.x;
        acc_y_sum += acc.y;
        acc_z_sum += acc.z;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGI(TAG, "Calibration complete: X=%.3f Y=%.3f Z=%.3f",
             acc_x_sum / 100.0f, acc_y_sum / 100.0f, acc_z_sum / 100.0f);

    return ESP_OK;
}

bool imu_driver_is_available(void)
{
    return s_initialized;
}

uint8_t imu_driver_get_device_id(void)
{
    uint8_t dev_id = 0;
    if (s_initialized) {
        imu_driver_read_register(QMI8658C_REG_WHO_AM_I, &dev_id);
    }
    return dev_id;
}
