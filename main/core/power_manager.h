/**
 * @file power_manager.h
 * @brief 电源管理模块 - 超长续航核心
 * 
 * 设计目标：7-10天续航
 * - 深度睡眠时功耗 < 100uA
 * - 1Hz 低频刷新
 * - 抬腕唤醒
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    POWER_MODE_ACTIVE,      // 正常模式（用户交互中）
    POWER_MODE_IDLE,        // 空闲模式（屏幕亮，无操作）
    POWER_MODE_SLEEP,       // 睡眠模式（屏幕关闭，BLE保持）
    POWER_MODE_DEEP_SLEEP,  // 深度睡眠（仅RTC运行，抬腕唤醒）
} power_mode_t;

// 初始化电源管理
esp_err_t power_manager_init(void);

// 设置电源模式
esp_err_t power_manager_set_mode(power_mode_t mode);

// 获取当前电源模式
power_mode_t power_manager_get_mode(void);

// 注册唤醒源（抬腕、按键、BLE通知）
esp_err_t power_manager_register_wakeup_source(uint32_t wakeup_sources);

// 进入深度睡眠（由系统自动调用）
esp_err_t power_manager_enter_deep_sleep(uint32_t sleep_ms);

// 获取电池电量百分比 (0-100)
uint8_t power_manager_get_battery_level(void);

// 是否正在充电
bool power_manager_is_charging(void);

// 用户活动标记（重置自动睡眠计时器）
void power_manager_user_activity(void);

#endif // POWER_MANAGER_H
