/**
 * @file event_bus.h
 * @brief 事件总线 - 解耦模块通信
 * 
 * 所有模块通过事件通信，避免直接调用
 */

#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <stdint.h>
#include "esp_err.h"

// 事件类型
typedef enum {
    // 系统事件
    EVENT_SYSTEM_BOOT,
    EVENT_SYSTEM_SHUTDOWN,
    EVENT_SYSTEM_LOW_BATTERY,
    
    // 电源事件
    EVENT_POWER_MODE_CHANGED,
    EVENT_POWER_ENTER_SLEEP,
    EVENT_POWER_WAKEUP,
    
    // 显示事件
    EVENT_DISPLAY_ON,
    EVENT_DISPLAY_OFF,
    EVENT_DISPLAY_UPDATE,
    
    // 输入事件
    EVENT_INPUT_BUTTON_PRESS,
    EVENT_INPUT_BUTTON_LONG_PRESS,
    EVENT_INPUT_WRIST_TILT,      // 抬腕
    EVENT_INPUT_WRIST_SHAKE,     // 摇腕
    
    // BLE事件
    EVENT_BLE_CONNECTED,
    EVENT_BLE_DISCONNECTED,
    EVENT_BLE_NOTIFICATION,      // 手机通知
    EVENT_BLE_SYNC_TIME,         // 同步时间
    
    // 传感器事件
    EVENT_SENSOR_HEART_RATE_READY,
    EVENT_SENSOR_STEP_COUNT,
    
    // 应用事件
    EVENT_APP_SHOW_TIME,
    EVENT_APP_SHOW_NOTIFICATION,
    EVENT_APP_SHOW_HEART_RATE,
    EVENT_APP_SHOW_SETTINGS,
} event_type_t;

// 事件数据结构
typedef struct {
    event_type_t type;
    uint32_t timestamp;
    union {
        uint32_t data_u32;
        int32_t data_i32;
        void *data_ptr;
    };
} event_t;

// 事件回调函数类型
typedef void (*event_handler_t)(const event_t *event);

// 初始化事件总线
esp_err_t event_bus_init(void);

// 订阅事件
esp_err_t event_bus_subscribe(event_type_t type, event_handler_t handler);

// 取消订阅
esp_err_t event_bus_unsubscribe(event_type_t type, event_handler_t handler);

// 发布事件
esp_err_t event_bus_publish(const event_t *event);

// 快捷发布（无数据）
esp_err_t event_bus_publish_simple(event_type_t type);

#endif // EVENT_BUS_H
