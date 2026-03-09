/**
 * @file test_touch.c
 * @brief CST816 Touch Screen Test Application
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_config.h"
#include "display_driver.h"

static const char *TAG = "CST816";

#define CST816_I2C_ADDR         TOUCH_I2C_ADDR
#define CST816_REG_CHIP_ID      0xA7
#define CST816_REG_STATUS       0xAB
#define CST816_REG_X_HIGH       0x03
#define CST816_REG_Y_HIGH       0x05
#define CST816_REG_X_LOW        0x04
#define CST816_REG_Y_LOW        0x06
#define CST816_REG_GESTURE      0x01
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

esp_err_t test_touch_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing CST816 touch controller");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TOUCH_RESET_GPIO) | (1ULL << TOUCH_INT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(TOUCH_RESET_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TOUCH_RESET_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << TOUCH_INT_GPIO);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&io_conf);

    uint8_t chip_id = 0;
    if (touch_read_register(CST816_REG_CHIP_ID, &chip_id) == ESP_OK) {
        ESP_LOGI(TAG, "Chip ID: 0x%02X (expected 0x%02X)", chip_id, CST816_CHIP_ID);
        if (chip_id != CST816_CHIP_ID) {
            ESP_LOGW(TAG, "Unexpected chip ID, but continuing...");
        }
    } else {
        ESP_LOGE(TAG, "Failed to read chip ID");
        return ESP_FAIL;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Touch controller initialized");
    return ESP_OK;
}

esp_err_t test_touch_read(touch_point_t *point)
{
    if (!s_initialized || !point) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t status = 0;
    esp_err_t ret = touch_read_register(CST816_REG_STATUS, &status);

    if (ret != ESP_OK) {
        return ret;
    }

    if ((status & 0x80) == 0) {
        point->pressed = false;
        point->x = 0;
        point->y = 0;
        return ESP_OK;
    }

    uint8_t data[4] = {0};
    ret = touch_read_registers(CST816_REG_X_HIGH, data, sizeof(data));
    if (ret != ESP_OK) {
        return ret;
    }

    point->x = ((data[0] & 0x0F) << 8) | data[1];
    point->y = ((data[2] & 0x0F) << 8) | data[3];
    point->pressed = true;

    return ESP_OK;
}

void test_touch_display_position(touch_point_t *point)
{
    if (!point->pressed) {
        return;
    }

    char text[64];
    snprintf(text, sizeof(text), "X:%d Y:%d", point->x, point->y);

    ESP_LOGI(TAG, "Touch: %s", text);
}

void test_touch_task(void *pvParameters)
{
    (void)pvParameters;

    touch_point_t point;

    while (1) {
        if (test_touch_read(&point) == ESP_OK) {
            if (point.pressed) {
                ESP_LOGI(TAG, "Touch detected at (%d, %d)", point.x, point.y);
                test_touch_display_position(&point);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void test_touch_run(void)
{
    if (test_touch_init() != ESP_OK) {
        ESP_LOGE(TAG, "Touch init failed");
        return;
    }

    xTaskCreate(test_touch_task, "touch_test", 4096, NULL, 5, NULL);
}
