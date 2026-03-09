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

#### 面向对象 C 编程

**使用结构体封装对象状态**：每个驱动/模块使用结构体保存私有状态
```c
typedef struct {
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    bool initialized;
    // ... 其他私有成员
} xxx_driver_t;
```

**使用单例模式**：每个驱动模块一个全局实例
```c
static xxx_driver_t s_driver = {0};
```

**函数指针封装（虚函数表模式）**：将方法作为函数指针封装在结构体中
```c
// 定义驱动接口（类似 C++ 的抽象基类）
typedef struct {
    esp_err_t (*init)(void *self, const config_t *config);
    esp_err_t (*deinit)(void *self);
    esp_err_t (*read)(void *self, uint8_t reg, uint8_t *buf, size_t len);
    esp_err_t (*write)(void *self, uint8_t reg, const uint8_t *buf, size_t len);
} xxx_driver_iface_t;

// 具体实现结构体包含接口和私有数据
typedef struct {
    const xxx_driver_iface_t *iface;  // 虚函数表
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    bool initialized;
} xxx_driver_impl_t;

// 实现函数
static esp_err_t xxx_init_impl(void *self, const config_t *config) {
    xxx_driver_impl_t *driver = (xxx_driver_impl_t *)self;
    // 实现代码
}

// 虚函数表实例
static const xxx_driver_iface_t xxx_driver_vtable = {
    .init = xxx_init_impl,
    .deinit = xxx_deinit_impl,
    .read = xxx_read_impl,
    .write = xxx_write_impl,
};

// 全局单例
static xxx_driver_impl_t s_driver = {
    .iface = &xxx_driver_vtable,
    .i2c_port = I2C_NUM_0,
    .i2c_addr = 0x3C,
};

// 使用方式（类似多态调用）
s_driver.iface->init(&s_driver, &config);
s_driver.iface->write(&s_driver, reg, data, len);
```

**方法命名规范**：`模块名_方法名 (对象，参数...)`
```c
esp_err_t display_driver_init(display_driver_t *driver, const config_t *config);
esp_err_t display_driver_write_command(display_driver_t *driver, uint8_t cmd);
```

**封装私有方法**：使用 `static` 限制访问范围

**提供构造函数/析构函数**：`xxx_init()` 和 `xxx_deinit()`

#### 日志规范
- **必须打印足够的日志**：每个关键步骤都要打印日志
  ```c
  ESP_LOGI(TAG, "Initializing %s...", DRIVER_NAME);
  ESP_LOGI(TAG, "I2C port: %d, address: 0x%02X", i2c_port, i2c_addr);
  ESP_LOGI(TAG, "%s initialized successfully", DRIVER_NAME);
  ```
- **错误日志必须包含**：
  - 错误发生位置
  - 错误代码 (`esp_err_t`)
  - 相关参数值
  ```c
  ESP_LOGE(TAG, "Failed to write register 0x%02X: %s (0x%X)", 
           reg, esp_err_to_name(ret), ret);
  ```
- **调试日志级别**：
  - `ESP_LOGI` - 正常流程（初始化完成、状态变化）
  - `ESP_LOGW` - 警告（非致命错误、降级处理）
  - `ESP_LOGE` - 错误（操作失败、硬件异常）
  - `ESP_LOGD` - 调试（详细数据、寄存器值，可条件编译）

#### 其他规范
- 函数命名：`snake_case`
- 结构体：`typedef struct { ... } xxx_t;`
- 宏定义：`ALL_CAPS_WITH_UNDERSCORE`
- 所有外设驱动封装为 `.c/.h` 文件，提供统一接口
- 任务栈大小：至少 4096 字节
- 错误处理：必须检查返回值

### 3. 功耗管理
- 空闲时进入轻睡眠：`esp_sleep_enable_timer_wakeup()`
- 屏幕关闭后关闭背光
- 音频播放结束后关闭功放
- IMU 和传感器不使用时关闭电源

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
