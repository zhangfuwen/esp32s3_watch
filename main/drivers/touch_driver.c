/**
 * @file touch_driver.c
 * @brief CST816 Touch Driver Implementation
 */

#include "touch_driver.h"
#include "board_config.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "CST816";

// Implementation functions
static esp_err_t touch_init_impl(void *self) {
    touch_driver_impl_t *driver = (touch_driver_impl_t *)self;
    
    ESP_LOGI(TAG, "Initializing CST816 touch driver...");
    ESP_LOGI(TAG, "I2C port: %d, address: 0x%02X, INT: GPIO%d", 
             driver->i2c_port, driver->i2c_addr, driver->int_pin);
    
    // Configure INT pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << driver->int_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_LOGI(TAG, "GPIO INT configured");
    
    // Read chip ID
    uint8_t chip_id = 0;
    uint8_t reg = CST816_CHIP_ID;
    esp_err_t ret = i2c_master_write_read_device(driver->i2c_port, driver->i2c_addr,
                                                  &reg, 1, &chip_id, 1, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read chip ID: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "CST816 chip ID: 0x%02X", chip_id);
    
    if (chip_id != 0xB5 && chip_id != 0xB6) {
        ESP_LOGW(TAG, "Unexpected chip ID (expected 0xB5 or 0xB6)");
    }
    
    driver->initialized = true;
    driver->last_x = 0;
    driver->last_y = 0;
    driver->last_pressed = false;
    
    ESP_LOGI(TAG, "CST816 touch driver initialized successfully");
    return ESP_OK;
}

static esp_err_t touch_deinit_impl(void *self) {
    touch_driver_impl_t *driver = (touch_driver_impl_t *)self;
    
    if (!driver->initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing CST816 touch driver");
    driver->initialized = false;
    return ESP_OK;
}

static esp_err_t touch_read_point_impl(void *self, uint16_t *x, uint16_t *y, bool *pressed) {
    touch_driver_impl_t *driver = (touch_driver_impl_t *)self;
    
    if (!driver->initialized || !x || !y || !pressed) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t buffer[6];
    buffer[0] = CST816_GEST_ID;
    
    esp_err_t ret = i2c_master_write_read_device(driver->i2c_port, driver->i2c_addr,
                                                  buffer, 1, buffer, 6, 100);
    if (ret != ESP_OK) {
        *pressed = false;
        return ret;
    }
    
    uint8_t finger_num = buffer[1];
    
    if (finger_num > 0) {
        *x = ((buffer[2] & 0x0F) << 8) | buffer[3];
        *y = ((buffer[4] & 0x0F) << 8) | buffer[5];
        *pressed = true;
        
        driver->last_x = *x;
        driver->last_y = *y;
        driver->last_pressed = true;
        
        ESP_LOGD(TAG, "Touch: x=%d, y=%d", *x, *y);
    } else {
        *pressed = false;
        *x = driver->last_x;
        *y = driver->last_y;
        driver->last_pressed = false;
    }
    
    return ESP_OK;
}

static void touch_lvgl_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    touch_driver_impl_t *driver = (touch_driver_impl_t *)drv->user_data;
    
    uint16_t x, y;
    bool pressed;
    
    esp_err_t ret = driver->iface->read_point(driver, &x, &y, &pressed);
    
    if (ret == ESP_OK) {
        data->point.x = x;
        data->point.y = y;
        data->state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// VTable instance
static const touch_driver_iface_t g_touch_vtable = {
    .init = touch_init_impl,
    .deinit = touch_deinit_impl,
    .read_point = touch_read_point_impl,
    .lvgl_read_cb = touch_lvgl_read_cb,
};

// Global instance definition
touch_driver_impl_t g_touch_driver = {
    .iface = &g_touch_vtable,
    .i2c_port = I2C_NUM_0,
    .i2c_addr = 0x15,
    .int_pin = GPIO_NUM_41,
    .initialized = false,
};

// Public API implementation
esp_err_t touch_driver_init(touch_driver_impl_t *driver, i2c_port_t i2c_port,
                            uint8_t i2c_addr, gpio_num_t int_pin) {
    driver->i2c_port = i2c_port;
    driver->i2c_addr = i2c_addr;
    driver->int_pin = int_pin;
    driver->iface = &g_touch_vtable;
    
    return driver->iface->init(driver);
}

esp_err_t touch_driver_deinit(touch_driver_impl_t *driver) {
    return driver->iface->deinit(driver);
}

esp_err_t touch_driver_read_point(touch_driver_impl_t *driver,
                                   uint16_t *x, uint16_t *y, bool *pressed) {
    return driver->iface->read_point(driver, x, y, pressed);
}

void touch_driver_register_lvgl(lv_indev_drv_t *indev_drv, touch_driver_impl_t *driver) {
    lv_indev_drv_init(indev_drv);
    indev_drv->type = LV_INDEV_TYPE_POINTER;
    indev_drv->read_cb = driver->iface->lvgl_read_cb;
    indev_drv->user_data = driver;
    
    lv_indev_t *touch_indev = lv_indev_drv_register(indev_drv);
    if (touch_indev) {
        ESP_LOGI(TAG, "LVGL touch input device registered");
    } else {
        ESP_LOGE(TAG, "Failed to register LVGL touch input device");
    }
}
