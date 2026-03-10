/**
 * @file battery_driver.h
 * @brief MAX17048G Battery Fuel Gauge Driver
 */

#ifndef BATTERY_DRIVER_H
#define BATTERY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// MAX17048G registers
#define MAX17048_VCELL    0x02
#define MAX17048_SOC      0x04  // State of Charge (%)
#define MAX17048_MODE     0x06
#define MAX17048_VER      0x08
#define MAX17048_CONFIG   0x0C
#define MAX17048_COMMAND  0xFE

// Interface structure (vtable)
typedef struct {
    esp_err_t (*init)(void *self);
    esp_err_t (*deinit)(void *self);
    esp_err_t (*get_soc)(void *self, uint8_t *soc);
    esp_err_t (*get_voltage)(void *self, uint16_t *voltage_mv);
} battery_driver_iface_t;

// Implementation structure
typedef struct {
    const battery_driver_iface_t *iface;
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    bool initialized;
    uint8_t last_soc;
    uint16_t last_voltage_mv;
} battery_driver_impl_t;

// Global instance
extern battery_driver_impl_t g_battery_driver;

// Public API
esp_err_t battery_driver_init(battery_driver_impl_t *driver, i2c_port_t i2c_port, uint8_t i2c_addr);
esp_err_t battery_driver_deinit(battery_driver_impl_t *driver);
esp_err_t battery_driver_get_soc(battery_driver_impl_t *driver, uint8_t *soc);
esp_err_t battery_driver_get_voltage(battery_driver_impl_t *driver, uint16_t *voltage_mv);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_DRIVER_H
