#!/bin/bash
# ESP32-S3 Watch - Build & Package Script
# 编译 + 打包 + 发送 Telegram

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

echo "🔧 ESP32-S3 Watch - Build & Package"
echo "===================================="

# 加载 ESP-IDF
if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    echo "📦 Loading ESP-IDF environment..."
    . $HOME/esp/esp-idf/export.sh 2>/dev/null
else
    echo "❌ ESP-IDF not found"
    exit 1
fi

# 编译
echo ""
echo "🔨 Building..."
idf.py build

# 检查编译
if [ ! -f "build/esp32-watch.bin" ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo ""
echo "✅ Build successful!"

# 打包
echo ""
echo "📦 Creating package..."
PACKAGE_DIR="dist"
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

cp build/bootloader/bootloader.bin "$PACKAGE_DIR/"
cp build/partition_table/partition-table.bin "$PACKAGE_DIR/"
cp build/esp32-watch.bin "$PACKAGE_DIR/"

# 刷机脚本
cat > "$PACKAGE_DIR/flash.sh" << 'FLASH'
#!/bin/bash
PORT="${1:-/dev/tty.usbmodem14101}"
echo "🔧 Flashing to $PORT..."
esptool.py --chip esp32s3 --port "$PORT" --baud 921600 write_flash \
    0x0000 bootloader.bin \
    0x8000 partition-table.bin \
    0x10000 esp32-watch.bin \
    --flash_mode dio --flash_freq 80m --flash_size 16MB
echo "✅ Done!"
FLASH
chmod +x "$PACKAGE_DIR/flash.sh"

# 说明
cat > "$PACKAGE_DIR/README.md" << README
# ESP32-S3 Watch Firmware

**Build:** $(date '+%Y-%m-%d %H:%M:%S')  
**Commit:** $(git rev-parse --short HEAD 2>/dev/null || echo 'unknown')

## Flash

\`\`\`bash
./flash.sh [port]
\`\`\`

Default: /dev/tty.usbmodem14101

## Monitor

\`\`\`bash
idf.py -p [port] monitor
\`\`\`
README

# 压缩包
cd "$PACKAGE_DIR"
ARCHIVE="../esp32-watch-$(date '+%Y%m%d-%H%M%S').zip"
zip -q "$ARCHIVE" *

echo "✅ Package: $ARCHIVE"
ls -lh "$ARCHIVE"

# 显示二进制大小
echo ""
echo "📊 Binary Sizes:"
ls -lh ../build/*.bin

echo ""
echo "===================================="
echo "✨ Done! Ready to flash."
