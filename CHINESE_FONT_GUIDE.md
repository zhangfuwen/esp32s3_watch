# 中文显示配置指南

## 现状

❌ **当前版本不支持中文显示**

原因：
- LVGL 默认只包含西文字体（Montserrat 系列）
- 中文字体文件很大（完整 GB2312 约 10MB+）
- ESP32-S3 的 Flash 只有 2MB（放不下完整中文字体）

---

## 解决方案

### 方案 1：点阵汉字（推荐 ⭐）

**优点：**
- 字体文件小（常用字约 50-200KB）
- 显示速度快
- 适合手表小屏幕

**缺点：**
- 只能显示预设的汉字
- 字体样式固定

**步骤：**

1. **生成点阵字库**
   
   使用 [取字模工具](http://www.lcdwiki.com/zh/Font_Displayer) 或 `fontconvert`：
   
   ```bash
   # 安装 fontconvert
   sudo apt-get install freetype2-dev
   
   # 生成 16px 宋体点阵（常用 100 字）
   ./fontconvert 16 "你好世界时间日期电池电量" > chinese_16.c
   ```

2. **添加到工程**

   ```c
   // main/fonts/chinese_16.c
   #include "lvgl.h"
   
   const lv_font_t chinese_16 = {
       .glyph_bitmap = ...,
       .glyph_dsc = ...,
       .line_height = 16,
       .base_line = 0,
   };
   ```

3. **使用中文**

   ```c
   lv_obj_t *label = lv_label_create(scr);
   lv_label_set_text(label, "你好");
   lv_obj_set_style_text_font(label, &chinese_16, 0);
   ```

**限制：** 只能显示预先生成的汉字。

---

### 方案 2：Unicode 解码 + 外置 Flash（复杂）

**优点：**
- 可显示任意汉字
- 字体美观

**缺点：**
- 需要外置 Flash 芯片（W25Qxx）
- 需要 FATFS 文件系统
- 运行时解码慢

**步骤：**

1. 启用 FATFS 和 SPIFFS
2. 将中文字体文件（.ttf/.ttc）放入 Flash
3. 使用 LVGL 的 `lv_ftfont` 模块动态加载

**不推荐：** 手表资源有限，复杂度太高。

---

### 方案 3：图片方案（简单）

**适用场景：** 固定短语（如"周一"、"电量"等）

**步骤：**

1. **制作图片**
   
   用 PS 制作 16×16 或 24×24 的中文 PNG 图片：
   ```
   images/mon.png  → "周一"
   images/tue.png  → "周二"
   images/battery.png → "电量"
   ```

2. **转换为 LVGL 格式**

   ```bash
   # 使用 online converter
   https://lvgl.io/tools/imageconverter
   ```

3. **使用图片**

   ```c
   LV_IMG_DECLARE(mon);
   lv_obj_t *img = lv_img_create(scr);
   lv_img_set_src(img, &mon);
   ```

**优点：** 简单可靠，不占字体资源。

---

## 推荐方案：英文 + 数字 + 图标

对于手表应用，建议：

```
时间：12:30:00     ✅ 数字 + 符号
日期：Mon 03/10    ✅ 英文缩写 + 数字
电量：75%          ✅ 数字 + 符号
步骤：1,234        ✅ 数字
心率：72 bpm       ✅ 数字 + 英文
```

**图标代替中文：**
- 🔋 代替 "电量"
- ⌚ 代替 "时间"
- 📅 代替 "日期"
- 🚶 代替 "步数"
- ❤️ 代替 "心率"

---

## 如果一定要中文

### 最小化方案（50KB）

只生成最常用的 50 个汉字：

```
一二三四五六七八九十百千万
年月日时分秒周
电池电量时间日期星期
步数心率温度湿度
开关设置返回确定取消
```

**生成命令：**

```bash
cd watch
mkdir -p fonts
cd fonts

# 下载思源黑体
wget https://github.com/adobe-fonts/source-han-sans/raw/release/OTF/SimplifiedChinese/SourceHanSansSC-Regular.otf

# 生成 16px 点阵
fontconvert SourceHanSansSC-Regular.otf 16 "一二三四五六七八九十百千万年月日时分秒周电池电量时间日期星期步数心率温度湿度开关设置返回确定取消" > chinese_16.c
```

**添加到 CMakeLists.txt：**

```cmake
SRCS
    "fonts/chinese_16.c"
```

**使用：**

```c
#include "fonts/chinese_16.c"

lv_obj_t *label = lv_label_create(scr);
lv_label_set_text(label, "电量");
lv_obj_set_style_text_font(label, &chinese_16, 0);
```

---

## 实际建议

对于 ESP32-S3 Watch 项目：

### ✅ 推荐（当前方案）

```
显示内容：
┌────────────────┐
│  12:30:00      │  ← 数字
│  Mon 03/10     │  ← 英文缩写
│  [████░] 75%   │  ← 图标 + 数字
│  UP: 120s      │  ← 英文
└────────────────┘
```

**优势：**
- 字体库小（~200KB）
- 显示清晰
- 国际化友好

### ❌ 不推荐

```
显示内容：
┌────────────────┐
│    12:30:00    │
│    周一 3/10   │  ← 中文
│  电量 [███]75% │  ← 中文
│  运行：120 秒   │  ← 中文
└────────────────┘
```

**问题：**
- 字体库大（+200KB）
- 占用 Flash 空间
- 显示效果不一定好（小字号中文模糊）

---

## 结论

**当前版本：** 不支持中文，建议用英文 + 图标

**如果必须中文：**
1. 使用点阵汉字（常用 50 字，约 50KB）
2. 或使用图片方案（固定短语）
3. 不建议完整中文字体（太大）

---

**最后更新：** 2026-03-10  
**版本：** v3.0.2
