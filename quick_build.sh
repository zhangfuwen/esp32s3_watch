#!/bin/bash
# ESP32-S3 Watch - Quick Build & Package Script
# 快速编译打包脚本

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

echo "🔧 ESP32-S3 Watch - Build & Package"
echo "===================================="
echo ""

# 加载 ESP-IDF
if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    echo "Loading ESP-IDF environment..."
    . $HOME/esp/esp-idf/export.sh 2>/dev/null
else
    echo "❌ ESP-IDF not found at ~/esp/esp-idf"
    echo "Please install ESP-IDF first."
    exit 1
fi

# 编译
echo ""
echo "📦 Building project..."
echo ""
idf.py build

# 检查编译结果
if [ ! -f "build/esp32-watch.bin" ]; then
    echo ""
    echo "❌ Build failed!"
    exit 1
fi

echo ""
echo "✅ Build successful!"
echo ""

# 打包
echo "📦 Creating package..."
PACKAGE_DIR="dist"
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

cp build/bootloader/bootloader.bin "$PACKAGE_DIR/"
cp build/partition_table/partition-table.bin "$PACKAGE_DIR/"
cp build/esp32-watch.bin "$PACKAGE_DIR/"

# 创建刷机脚本
cat > "$PACKAGE_DIR/flash.sh" << 'EOF'
#!/bin/bash
PORT="${1:-/dev/tty.usbmodem14101}"
echo "Flashing to $PORT..."
esptool.py --chip esp32s3 --port "$PORT" --baud 921600 write_flash \
    0x0000 bootloader.bin \
    0x8000 partition-table.bin \
    0x10000 esp32-watch.bin \
    --flash_mode dio --flash_freq 80m --flash_size 16MB
echo "Done!"
EOF
chmod +x "$PACKAGE_DIR/flash.sh"

# 创建说明文件
cat > "$PACKAGE_DIR/README.txt" << EOF
ESP32-S3 Watch Firmware
Build: $(date '+%Y-%m-%d %H:%M:%S')

Files:
- bootloader.bin (0x0000)
- partition-table.bin (0x8000)
- esp32-watch.bin (0x10000)

Flash (macOS/Linux):
  ./flash.sh [port]
  
Flash (Windows):
  flash.bat [COM_PORT]

Default port: /dev/tty.usbmodem14101
EOF

# 创建压缩包
cd "$PACKAGE_DIR"
ARCHIVE="../esp32-watch-$(date '+%Y%m%d-%H%M%S').zip"
zip -q "$ARCHIVE" *

echo ""
echo "✅ Package created: $ARCHIVE"
echo ""
ls -lh "$ARCHIVE"
echo ""
echo "📁 Package contents:"
unzip -l "$ARCHIVE"

echo ""
echo "===================================="
echo "✨ Done! Ready to flash."
echo ""
echo "To flash:"
echo "  cd dist && ./flash.sh"
echo ""
echo "To send via Telegram, set env vars:"
echo "  export TELEGRAM_CHAT_ID=xxx"
echo "  export TELEGRAM_BOT_TOKEN=xxx"
echo "  Then run: send_telegram.sh"
