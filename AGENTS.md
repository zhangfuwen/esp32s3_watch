# 🤖 AGENTS.md - ESP32-S3 Watch Project

> **角色定位**：资深嵌入式工程师，专精于基于 ESP-IDF 的 ESP32-S3 手表开发。确保架构清晰、功耗可控、可维护性强，与硬件设计完全一致。

---

## 📝 一、项目概览

| 项目 | 规格 |
|------|------|
| **项目名称** | ESP32-Watch |
| **硬件平台** | ESPWatch-S3B2 (ESP32-S3 + 1.83" ST7789V2 IPS 屏) |
| **主控芯片** | ESP32-S3 (32 位 LX7 双核处理器) |
| **存储** | 16MB Flash, 支持 MicroSD 卡 |
| **屏幕** | 1.83 英寸 ST7789V2 IPS，240×285，262K 彩色 |
| **通信** | Bluetooth 5, Wi-Fi 6 (2.4GHz) |
| **电池** | 300mAh 锂电池，支持充电管理 |
| **音频** | ES8311 编码器 + NS4150 功放 + ZTS6216 麦克风 |

**传感器：**
- [CPU] 32 位 LX7 双核处理器
- [QMI8658C] 六轴惯性计 (IMU)
- [MAX30102] 心率/血氧传感器
- [MAX17048G] 电量计芯片
- [CST816] 电容触摸芯片
- [ES8311] 音频编码器
- [振动马达] 触觉反馈

**软件架构：** HAL → Drivers → Services → Apps，事件驱动 + 任务调度，避免阻塞。

---

## 📚 二、技术栈与版本锁定

| 功能 | 组件 | 版本 |
|------|------|------|
| **核心框架** | ESP-IDF | v5.2.1 (禁止混用 Arduino) |
| **GUI** | LVGL | ESP-IDF 内置 |
| **JSON** | cJSON | ESP-IDF 内置 |
| **文件系统** | fatfs | ESP-IDF 内置 |
| **NVS 存储** | nvs | ESP-IDF 内置 |

---

## 🔌 三、引脚定义

### 核心引脚映射

| 功能 | 引脚 | GPIO |
|------|------|------|
| **按键** | BOOT | IO0 |
| | PWR | IO45 |
| **I2C 总线** | SDA | IO1 |
| | SCL | IO2 |
| **显示屏** | SPI_CS | IO12 |
| (ST7789) | SPI_SCL | IO5 |
| | SPI_MOSI | IO6 |
| | SPI_DC | IO11 |
| | 背光 | IO47 |
| **音频** | I2S_MCLK | IO21 |
| (ES8311) | I2S_WS | IO14 |
| | I2S_BCLK | IO18 |
| | I2S_DI | IO17 |
| | I2S_DO | IO13 |
| | PA_EN | IO48 |
| **IMU** | INT1 | IO41 |
| (QMI8658C) | INT2 | IO42 |
| **触摸屏** | RST | IO7 |
| (CST816) | INT | IO46 |
| **其他** | 振动马达 | IO4 |
| | SD_DAT0 | IO10 |
| | SD_CMD | IO8 |
| | SD_CLK | IO9 |
| | USB D- | IO19 |
| | USB D+ | IO20 |

### 屏幕参数
```c
#define DISPLAY_WIDTH       240
#define DISPLAY_HEIGHT      284
#define DISPLAY_SWAP_XY     false
#define DISPLAY_MIRROR_X    false
#define DISPLAY_MIRROR_Y    false
#define DISPLAY_INVERT_COLOR false
```

---

## 🧩 四、核心系统库

### 1. FreeRTOS (实时操作系统)
- 所有应用逻辑在独立任务中运行，避免阻塞主循环
- 使用 `xTaskCreate()` 创建任务，`vTaskDelete()` 删除
- 使用 `esp_timer` 实现高精度延时

### 2. Wi-Fi (esp_wifi)
```c
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>

wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);
esp_wifi_set_mode(WIFI_MODE_STA);
esp_wifi_start();
```

### 3. BLE (esp_bt)
```c
#include <esp_bt.h>
#include <esp_gatt_defs.h>
#include <esp_gattc_api.h>
#include <esp_gatts_api.h>

esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
esp_bt_controller_init(&bt_cfg);
esp_bt_controller_enable(ESP_BT_MODE_BLE);
```

### 4. 其他核心库
| 库 | 头文件 | 用途 |
|----|--------|------|
| ADC | `<driver/adc.h>` | 电池电压测量 |
| I2C | `<driver/i2c.h>` | 传感器通信 |
| SPI | `<driver/spi_master.h>` | 显示屏通信 |
| GPIO | `<driver/gpio.h>` | 通用 IO 控制 |
| 定时器 | `<driver/timer.h>` 或 `<esp_timer.h>` | 定时任务 |

---

## 📜 五、开发规范

### 1. 项目结构
```
esp32-watch/
├── main/
│   ├── core/           # 框架层 (event_bus, power_manager)
│   ├── drivers/        # 硬件驱动 (display, imu, audio, ble)
│   ├── services/       # 业务服务 (time, notifications, recorder)
│   ├── ui/             # 用户界面 (watch_face, menus)
│   └── main.c          # 应用入口
├── include/
│   └── board_config.h  # 硬件引脚定义
├── config/
│   ├── hardware_pins.h # 引脚配置模板
│   └── system_config.h # 系统配置
├── CMakeLists.txt
└── README.md
```

### 2. 编码规范
- 函数命名：`snake_case`
- 结构体：`typedef struct { ... } xxx_t;`
- 宏定义：`ALL_CAPS_WITH_UNDERSCORE`
- 所有外设驱动封装为 `.c/.h` 文件，提供统一接口
- 任务栈大小：至少 4096 字节
- 日志：使用 `ESP_LOGI/W/E`，禁止 `printf`
- 错误处理：必须检查返回值

### 3. 功耗管理
- 空闲时进入轻睡眠：`esp_sleep_enable_timer_wakeup()`
- 屏幕关闭后关闭背光
- 音频播放结束后关闭功放
- IMU 和传感器不使用时关闭电源

---

## ✅ 六、OpenCode 交互指南

### 长期会话最佳实践
1. **使用 ACP 协议**：`opencode acp` 保持持久会话
2. **保存 Session ID**：使用 `session/load` 继续对话
3. **避免重复解释**：将上下文存储在 AGENTS.md 或项目文档中
4. **批量任务**：给出完整任务列表，而非逐个指令
5. **检查进度**：每 30-60 秒轮询一次，不要连续询问

### 有用的第三方库
- **ESP-IDF Components**: https://components.espressif.com/
- **lvgl/lv_port_esp32**: GUI 库
- **ESP32-TimeLib**: RTC 时间管理 + NTP 同步
- **ESP32-BLE-Kit**: BLE GATT 模板
- **ESP32-AudioI2S**: I2S 音频库 (ES8311/ES8388)
- **ESP32-LowPower**: 深度睡眠优化示例

---

## 🛠️ 七、构建与烧录

```bash
# 构建
idf.py build

# 烧录 (替换为你的串口)
idf.py -p /dev/tty.usbmodem14101 flash monitor

# 仅监控
idf.py -p /dev/tty.usbmodem14101 monitor
```

---

> **最后更新**: 2026-03-08
> **硬件版本**: ESPWatch-S3B2
> **ESP-IDF 版本**: v5.2.1
