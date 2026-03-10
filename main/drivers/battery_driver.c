/**
 * @file battery_driver.c
 * @brief MAX17048G Battery Driver Implementation
 */

#include "battery_driver.h"
#include "esp_log.h"

static const char *TAG = "MAX17048";

// Implementation functions
static esp_err_t battery_init_impl(void *self) {
    battery_driver_impl_t *driver = (battery_driver_impl_t *)self;
    
    ESP_LOGI(TAG, "Initializing MAX17048G battery driver...");
    ESP_LOGI(TAG, "I2C port: %d, address: 0x%02X", driver->i2c_port, driver->i2c_addr);
    
    // Read version
    uint8_t reg = MAX17048_VER;
    uint16_t version = 0;
    uint8_t buffer[2];
    
    esp_err_t ret = i2c_master_write_read_device(driver->i2c_port, driver->i2c_addr,
                                                  &reg, 1, buffer, 2, 100);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read version: %s (device may not be connected)", esp_err_to_name(ret));
        driver->initialized = true;  // Continue anyway
        return ret;
    }
    
    version = (buffer[0] << 8) | buffer[1];
    ESP_LOGI(TAG, "MAX17048G version: 0x%04X", version);
    
    driver->initialized = true;
    driver->last_soc = 0;
    driver->last_voltage_mv = 0;
    
    ESP_LOGI(TAG, "MAX17048G battery driver initialized");
    return ESP_OK;
}

static esp_err_t battery_deinit_impl(void *self) {
    battery_driver_impl_t *driver = (battery_driver_impl_t *)self;
    
    if (!driver->initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing MAX17048G battery driver");
    driver->initialized = false;
    return ESP_OK;
}

static esp_err_t battery_get_soc_impl(void *self, uint8_t *soc) {
    battery_driver_impl_t *driver = (battery_driver_impl_t *)self;
    
    if (!driver->initialized || !soc) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t reg = MAX17048_SOC;
    uint8_t buffer[2];
    
    esp_err_t ret = i2c_master_write_read_device(driver->i2c_port, driver->i2c_addr,
                                                  &reg, 1, buffer, 2, 100);
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "Failed to read SOC: %s", esp_err_to_name(ret));
        *soc = driver->last_soc;
        return ret;
    }
    
    // SOC is in upper 8 bits (lower 8 bits are fractional)
    *soc = buffer[0];
    driver->last_soc = *soc;
    
    ESP_LOGD(TAG, "Battery SOC: %d%%", *soc);
    return ESP_OK;
}

static esp_err_t battery_get_voltage_impl(void *self, uint16_t *voltage_mv) {
    battery_driver_impl_t *driver = (battery_driver_impl_t *)self;
    
    if (!driver->initialized || !voltage_mv) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t reg = MAX17048_VCELL;
    uint8_t buffer[2];
    
    esp_err_t ret = i2c_master_write_read_device(driver->i2c_port, driver->i2c_addr,
                                                  &reg, 1, buffer, 2, 100);
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "Failed to read voltage: %s", esp_err_to_name(ret));
        *voltage_mv = driver->last_voltage_mv;
        return ret;
    }
    
    // VCELL is 12-bit value, LSB = 78.125uV
    uint16_t vcell = ((buffer[0] << 8) | buffer[1]) >> 4;
    *voltage_mv = (vcell * 78125) / 10000;  // Convert to mV
    driver->last_voltage_mv = *voltage_mv;
    
    ESP_LOGD(TAG, "Battery voltage: %d mV", *voltage_mv);
    return ESP_OK;
}

// VTable instance
static const battery_driver_iface_t g_battery_vtable = {
    .init = battery_init_impl,
    .deinit = battery_deinit_impl,
    .get_soc = battery_get_soc_impl,
    .get_voltage = battery_get_voltage_impl,
};

// Global instance definition
battery_driver_impl_t g_battery_driver = {
    .iface = &g_battery_vtable,
    .i2c_port = I2C_NUM_0,
    .i2c_addr = 0x36,
    .initialized = false,
};

// Public API implementation
esp_err_t battery_driver_init(battery_driver_impl_t *driver, i2c_port_t i2c_port, uint8_t i2c_addr) {
    driver->i2c_port = i2c_port;
    driver->i2c_addr = i2c_addr;
    driver->iface = &g_battery_vtable;
    
    return driver->iface->init(driver);
}

esp_err_t battery_driver_deinit(battery_driver_impl_t *driver) {
    return driver->iface->deinit(driver);
}

esp_err_t battery_driver_get_soc(battery_driver_impl_t *driver, uint8_t *soc) {
    return driver->iface->get_soc(driver, soc);
}

esp_err_t battery_driver_get_voltage(battery_driver_impl_t *driver, uint16_t *voltage_mv) {
    return driver->iface->get_voltage(driver, voltage_mv);
}
