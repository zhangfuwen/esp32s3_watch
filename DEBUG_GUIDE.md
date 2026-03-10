# ESP32-S3 Watch 开发调试指南

> 血泪经验总结，避免踩坑 🩸

---

## 📋 目录

1. [LVGL 显示问题](#lvgl-显示问题)
2. [字体配置](#字体配置)
3. [SPI 显示驱动](#spi 显示驱动)
4. [抬腕检测](#抬腕检测)
5. [功耗优化](#功耗优化)
6. [编译构建](#编译构建)
7. [调试技巧](#调试技巧)

---

## LVGL 显示问题

### ❌ 常见症状

- **半屏白屏，半屏花屏**
- **显示内容不完整**
- **屏幕闪烁或撕裂**

### 🔍 根本原因

**LVGL 缓冲区太小！**

```c
// ❌ 错误：缓冲区只有 28% 屏幕
#define LVGL_BUF_SIZE (DISPLAY_WIDTH * 80)  // 19200 像素

// ✅ 正确：缓冲区至少 50% 屏幕
#define LVGL_BUF_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 2)  // 34080 像素
```

**计算公式：**
```
屏幕总像素 = DISPLAY_WIDTH × DISPLAY_HEIGHT
          = 240 × 284 = 68,160 像素

最小缓冲区 = 屏幕总像素 × 50% = 34,080 像素
```

### ✅ 正确做法

```c
// lvgl_port.c
#define LVGL_BUF_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 2)

// 使用 DMA 内存
buf1 = heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
buf2 = heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

// 初始化双缓冲
lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LVGL_BUF_SIZE);
```

### 📊 缓冲区大小对比

| 缓冲区大小 | 占比 | 结果 |
|-----------|------|------|
| 19,200 (240×80) | 28% | ❌ 花屏/白屏 |
| 34,080 (240×142) | 50% | ✅ 正常 |
| 68,160 (全屏) | 100% | ✅ 最佳但占内存 |

---

## 字体配置

### ❌ 常见错误

```c
// ❌ 使用未启用的字体
lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
// 编译错误：'lv_font_montserrat_48' undeclared
```

### ✅ 正确步骤

**1. 修改 sdkconfig 启用字体**

```bash
# 手动编辑 sdkconfig 或使用 menuconfig
CONFIG_LV_FONT_MONTSERRAT_48=y
CONFIG_LV_FONT_MONTSERRAT_32=y
CONFIG_LV_FONT_MONTSERRAT_28=y
CONFIG_LV_FONT_MONTSERRAT_24=y
CONFIG_LV_FONT_MONTSERRAT_16=y
```

**2. 使用 sed 快速启用**

```bash
cd watch
sed -i.bak 's/# CONFIG_LV_FONT_MONTSERRAT_48 is not set/CONFIG_LV_FONT_MONTSERRAT_48=y/' sdkconfig
sed -i.bak 's/# CONFIG_LV_FONT_MONTSERRAT_32 is not set/CONFIG_LV_FONT_MONTSERRAT_32=y/' sdkconfig
# ... 其他字体
```

**3. 验证配置**

```bash
grep "LV_FONT_MONTSERRAT" sdkconfig | grep "=y"
```

### 📏 字体大小选择

| 用途 | 推荐大小 | 内存占用 |
|------|---------|---------|
| 时间显示 | 48px | ~40KB |
| 日期/标题 | 32px | ~25KB |
| 次要信息 | 24px | ~15KB |
| 状态栏 | 16px | ~8KB |

**总字体库占用：~200KB**

---

## SPI 显示驱动

### ❌ 常见错误

**1. 重复初始化 SPI 总线**

```c
// ❌ 错误：在多个地方调用 spi_bus_initialize()
display_driver_init();  // 初始化一次
lvgl_init_system();     // 又初始化一次 → ESP_ERR_INVALID_STATE
```

**2. 颜色顺序错误**

```c
// ❌ 错误：ST7789 使用 BGR 顺序
.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,

// ✅ 正确：ST7789 使用 BGR
.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
```

### ✅ 正确做法

**单一初始化路径：**

```c
// main.c
void app_main(void) {
    // 只调用一次 lvgl_init_system()
    lvgl_init_system();  // 包含 SPI、面板、LVGL 初始化
    lvgl_start_tasks();
    
    // 不要调用 display_driver_init() 或其他显示初始化
}
```

**ST7789 配置：**

```c
const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = DISPLAY_RESET_PIN,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,  // ⚠️ BGR!
    .bits_per_pixel = 16,
};
```

### 🔧 关键参数

| 参数 | 值 | 说明 |
|------|-----|------|
| SPI 时钟 | 20MHz | 过高会花屏 |
| 颜色顺序 | BGR | ST7789 专用 |
| 像素格式 | 16bit | RGB565 |
| DC 引脚 | GPIO11 | 数据/命令选择 |
| CS 引脚 | GPIO12 | 片选 |

---

## 抬腕检测

### ❌ 错误算法

```c
// ❌ 错误：检测加速度绝对值
float acc_mag = sqrt(x*x + y*y + z*z);  // 静止时=1.0g
if (acc_mag > 0.8g) wake();  // 1.0g > 0.8g，一直触发！
```

**问题：** 静止时重力就是 1g，逻辑混乱。

### ✅ 正确算法

```c
// ✅ 正确：检测加速度变化量（delta）
float delta_x = current_x - last_x;
float delta_y = current_y - last_y;
float delta_z = current_z - last_z;

float delta_mag = sqrt(delta_x² + delta_y² + delta_z²);

if (delta_mag * 1000 > 300) {  // 300mg 阈值
    wake();  // 只有移动才触发
}
```

### 📊 检测逻辑

```
静止：加速度变化≈0 → 不触发 ✅
抬手：加速度变化>300mg → 触发 ✅
放下：2 秒防抖 → 不重复触发 ✅
```

### 🔧 关键参数

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| 阈值 | 300mg | 灵敏度 |
| 防抖 | 2000ms | 避免重复触发 |
| 采样率 | 50Hz | 20ms 间隔 |

---

## 功耗优化

### ❌ 错误做法

```c
// ❌ 只隐藏 LVGL 对象，背光还亮着
lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
// 背光 GPIO 仍然是高电平 → 不省电
```

### ✅ 正确做法

```c
// ✅ 同时关闭背光 GPIO
void display_turn_off(void) {
    lv_obj_add_flag(screen_container, LV_OBJ_FLAG_HIDDEN);
    gpio_set_level(DISPLAY_BACKLIGHT_PIN, 0);  // ⚠️ 关键！
    display_on = false;
}

void display_turn_on(void) {
    lv_obj_clear_flag(screen_container, LV_OBJ_FLAG_HIDDEN);
    gpio_set_level(DISPLAY_BACKLIGHT_PIN, 1);
    display_on = true;
}
```

### 📊 功耗对比

| 状态 | 电流 | 说明 |
|------|------|------|
| 亮屏 | ~80mA | 背光开启 |
| 休眠（仅隐藏） | ~80mA | ❌ 背光未关 |
| 休眠（关背光） | ~5mA | ✅ 真正省电 |

---

## 编译构建

### ❌ 常见错误

**1. 格式说明符错误**

```c
// ❌ 错误：ESP_LOG 需要 PRI 家族宏
ESP_LOGI(TAG, "Count: %d", uint32_count);
// 警告：format '%d' expects argument of type 'int'

// ✅ 正确：使用 PRIu32
#include <inttypes.h>
ESP_LOGI(TAG, "Count: %" PRIu32, uint32_count);
```

**2. 隐式函数声明**

```c
// ❌ 错误：未包含头文件
lvgl_test_user_activity();  // implicit declaration

// ✅ 正确：添加头文件或前向声明
#include "lvgl_test.h"
// 或
extern void lvgl_test_user_activity(void);
```

**3. LVGL API 版本错误**

```c
// ❌ LVGL 8.x 不存在的 API
disp_drv.flush_wait_cb = flush_callback;  // 编译错误
disp_drv.flush_last = 1;  // 编译错误

// ✅ LVGL 8.x 正确用法
// 移除这些成员，不需要
```

### ✅ 构建命令

```bash
# 标准构建
cd watch
. ~/esp/esp-idf/export.sh > /dev/null 2>&1
idf.py build

# 查看编译错误
idf.py build 2>&1 | tee /tmp/build.log | tail -30

# 清理重建
idf.py fullclean
idf.py build
```

---

## 调试技巧

### 1. 串口日志

```bash
# 使用 idf_monitor
idf.py -p /dev/tty.usbmodem14101 monitor

# 或使用 screen
screen /dev/tty.usbmodem14101 115200

# 退出 screen: Ctrl+A, 然后 K, 然后 Y
```

### 2. 关键日志点

```c
// 在关键位置添加日志
ESP_LOGI(TAG, "Step 1: Screen created");
ESP_LOGI(TAG, "Step 2: Container created");
ESP_LOGI(TAG, "Step 3: Labels created");
ESP_LOGI(TAG, "Step 4: LVGL started");
```

**正常启动日志：**
```
I (xxx) LVGL: lv_init() OK
I (xxx) LVGL: SPI OK (20MHz)
I (xxx) LVGL: Panel OK
I (xxx) LVGL: Reset OK
I (xxx) LVGL: Init OK
I (xxx) LVGL: Buffers allocated
I (xxx) WATCH_FACE: Creating watch face...
I (xxx) WATCH_FACE: Screen created
I (xxx) WATCH_FACE: Container created
I (xxx) WATCH_FACE: Time labels created
I (xxx) WATCH_FACE: Watch face init complete
```

### 3. 二分法定位问题

如果显示异常：

1. **注释掉一半 UI 代码** → 编译测试
2. **如果正常** → 问题在注释的部分
3. **如果异常** → 问题在未注释的部分
4. **重复步骤** → 定位到具体代码

### 4. 内存调试

```c
// 检查可用内存
ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
ESP_LOGI(TAG, "Free PSRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

// 检查缓冲区分配
ESP_LOGI(TAG, "Buffer alloc: buf1=%p, buf2=%p", buf1, buf2);
if (!buf1 || !buf2) {
    ESP_LOGE(TAG, "Buffer alloc failed!");
}
```

### 5. 使用逻辑分析仪

如果有硬件问题（花屏、触摸不灵）：

- **SPI 波形** → 检查时钟、数据信号
- **I2C 波形** → 检查传感器通信
- **GPIO 波形** → 检查背光、中断引脚

---

## 快速参考

### 🚨 遇到问题先检查

1. **花屏/白屏** → LVGL 缓冲区大小
2. **字体编译错误** → sdkconfig 启用字体
3. **显示不亮** → 背光 GPIO、SPI 初始化
4. **抬腕不触发** → 检测算法（用 delta 不是绝对值）
5. **耗电快** → 背光是否真正关闭
6. **编译警告** → 格式说明符用 PRI 家族

### ✅ 标准流程

```bash
# 1. 修改代码
vim main/ui/watch_face.c

# 2. 编译
idf.py build 2>&1 | tee /tmp/build.log

# 3. 查看错误
tail -30 /tmp/build.log

# 4. 烧录
idf.py -p /dev/tty.usbmodem14101 flash

# 5. 监控日志
idf.py -p /dev/tty.usbmodem14101 monitor

# 6. 测试功能
# - 抬腕亮屏
# - 自动休眠
# - 背光关闭
```

### 📦 发布流程

```bash
# 1. 编译
idf.py build

# 2. 打包
cd dist
cp ../build/bootloader/bootloader.bin .
cp ../build/partition_table/partition-table.bin .
cp ../build/esp32_s3_watch.bin .
zip -r ../esp32-watch-v3.0.3.zip *

# 3. 创建 Release
gh release create v3.0.3 --title "版本名" --notes "更新说明" \
  ../build/bootloader/bootloader.bin \
  ../build/partition_table/partition-table.bin \
  ../build/esp32_s3_watch.bin

# 4. 清理
cd ..
rm -rf dist
```

---

## 总结

### 🎯 核心原则

1. **LVGL 缓冲区 ≥ 50% 屏幕**
2. **字体必须在 sdkconfig 启用**
3. **SPI 总线只能初始化一次**
4. **抬腕检测用变化量，不用绝对值**
5. **休眠必须关闭背光 GPIO**
6. **ESP_LOG 格式用 PRI 家族宏**

### 📝 检查清单

开发新功能时对照检查：

- [ ] LVGL 缓冲区足够大？
- [ ] 字体已启用？
- [ ] 没有重复初始化？
- [ ] 背光控制正确？
- [ ] 日志格式正确？
- [ ] 内存分配检查？
- [ ] 错误处理完善？

---

**最后更新：** 2026-03-10  
**版本：** v3.0.2  
**作者：** ESP32 开发团队
