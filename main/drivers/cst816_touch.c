/**
 * @file touch_driver.c
 * @brief CST816 Touch Screen Driver for LVGL
 */

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#include "board_config.h"

static const char *TAG = "CST816";

#define CST816_I2C_ADDR         0x15
#define CST816_REG_STATUS       0x02
#define CST816_REG_X_HIGH       0x03
#define CST816_REG_Y_HIGH       0x05
#define CST816_REG_X_LOW        0x04
#define CST816_REG_Y_LOW        0x06
#define CST816_CHIP_ID          0xB5

typedef struct {
    uint16_t x;
    uint16_t y;
    bool pressed;
} touch_point_t;

static bool s_initialized = false;

static esp_err_t touch_write_register(uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CST816_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t touch_read_register(uint8_t reg, uint8_t *value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CST816_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CST816_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, value, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t touch_read_registers(uint8_t reg, uint8_t *buffer, size_t length)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CST816_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CST816_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &buffer[length - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t cst816_touch_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // 等待 I2C 总线稳定
    ESP_LOGE(TAG, "Waiting 3 seconds for I2C bus to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGE(TAG, "========== TOUCH INIT START ==========");
    ESP_LOGE(TAG, "CST816 Touch Controller");
    ESP_LOGE(TAG, "I2C Port: %d", I2C_MASTER_NUM);
    ESP_LOGE(TAG, "I2C SDA: GPIO%d", I2C_MASTER_SDA_IO);
    ESP_LOGE(TAG, "I2C SCL: GPIO%d", I2C_MASTER_SCL_IO);
    ESP_LOGE(TAG, "RESET: GPIO%d", TOUCH_RESET_GPIO);
    ESP_LOGE(TAG, "INT: GPIO%d", TOUCH_INT_GPIO);
    ESP_LOGE(TAG, "I2C Addr: 0x%02X", CST816_I2C_ADDR);
    
    vTaskDelay(pdMS_TO_TICKS(500));

    // Configure RESET and INT pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TOUCH_RESET_GPIO) | (1ULL << TOUCH_INT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Reset touch controller
    ESP_LOGE(TAG, "Resetting CST816...");
    gpio_set_level(TOUCH_RESET_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TOUCH_RESET_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    // Scan I2C bus first
    ESP_LOGE(TAG, "Scanning I2C bus...");
    bool found = false;
    for (uint8_t addr = 0x00; addr <= 0x7F; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            ESP_LOGE(TAG, "Found device at 0x%02X", addr);
            found = true;
        }
    }
    
    if (!found) {
        ESP_LOGE(TAG, "No I2C devices found! Check wiring!");
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Read status register to verify communication
    ESP_LOGE(TAG, "Reading status register from 0x%02X...", CST816_I2C_ADDR);
    uint8_t status;
    esp_err_t ret = touch_read_register(CST816_REG_STATUS, &status);
    
    vTaskDelay(pdMS_TO_TICKS(500));
    
    if (ret == ESP_OK) {
        ESP_LOGE(TAG, "I2C OK! Status=0x%02X", status);
        ESP_LOGE(TAG, "CST816 communication verified!");
        s_initialized = true;
        ESP_LOGE(TAG, "========== TOUCH INIT SUCCESS ==========");
        vTaskDelay(pdMS_TO_TICKS(500));
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "I2C READ FAILED! Error: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check I2C wiring!");
        ESP_LOGE(TAG, "========== TOUCH INIT FAILED ==========");
        vTaskDelay(pdMS_TO_TICKS(500));
        return ret;
    }
}

esp_err_t cst816_touch_read(uint16_t *x, uint16_t *y, bool *pressed)
{
    if (!s_initialized) {
        *pressed = false;
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t status;
    esp_err_t ret = touch_read_register(CST816_REG_STATUS, &status);
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "Read status failed: %s", esp_err_to_name(ret));
        *pressed = false;
        return ret;
    }

    ESP_LOGV(TAG, "Touch status: 0x%02X", status);

    // Check if touch event (bits 0-2)
    if (status & 0x07) {
        uint8_t data[4];
        ret = touch_read_registers(CST816_REG_X_HIGH, data, sizeof(data));
        if (ret == ESP_OK) {
            *x = ((data[0] & 0x0F) << 8) | data[1];
            *y = ((data[2] & 0x0F) << 8) | data[3];
            *pressed = true;
            
            ESP_LOGI(TAG, "Touch: x=%d, y=%d, status=0x%02X", *x, *y, status);
            return ESP_OK;
        } else {
            ESP_LOGD(TAG, "Read touch data failed: %s", esp_err_to_name(ret));
        }
    }

    *pressed = false;
    return ESP_OK;
}
