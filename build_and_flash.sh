#!/bin/bash
# ESP32-S3 Watch Build & Flash Script
# 编译、打包、刷机一体化脚本

set -e

# 配置
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
PACKAGE_DIR="$PROJECT_DIR/dist"
SERIAL_PORT="${SERIAL_PORT:-/dev/tty.usbmodem14101}"
PROJECT_NAME="esp32-watch"
VERSION="${VERSION:-$(git describe --tags --always --dirty 2>/dev/null || echo 'dev')}"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 清理旧文件
clean() {
    log_info "Cleaning build directory..."
    rm -rf "$PACKAGE_DIR"
    mkdir -p "$PACKAGE_DIR"
}

# 编译项目
build() {
    log_info "Building ESP32-S3 project..."
    cd "$PROJECT_DIR"
    
    # 检查 ESP-IDF 环境
    if [ -z "$IDF_PATH" ]; then
        log_error "ESP-IDF environment not set. Please run: . $HOME/esp/esp-idf/export.sh"
        exit 1
    fi
    
    # 编译
    idf.py build
    
    # 检查编译结果
    if [ ! -f "$BUILD_DIR/$PROJECT_NAME.bin" ]; then
        log_error "Build failed! $PROJECT_NAME.bin not found"
        exit 1
    fi
    
    log_info "Build successful!"
}

# 打包固件
package() {
    log_info "Packaging firmware..."
    
    # 复制所有必要的 bin 文件
    cp "$BUILD_DIR/bootloader/bootloader.bin" "$PACKAGE_DIR/"
    cp "$BUILD_DIR/partition_table/partition-table.bin" "$PACKAGE_DIR/"
    cp "$BUILD_DIR/$PROJECT_NAME.bin" "$PACKAGE_DIR/"
    
    # 创建打包脚本
    cat > "$PACKAGE_DIR/flash.sh" << 'FLASH_SCRIPT'
#!/bin/bash
# ESP32-S3 Flash Script
# 使用方法：./flash.sh [串口端口]
# 默认端口：/dev/tty.usbmodem14101

SERIAL_PORT="${1:-/dev/tty.usbmodem14101}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Flashing ESP32-S3 Watch..."
echo "Serial port: $SERIAL_PORT"
echo ""

# 检查串口
if [ ! -c "$SERIAL_PORT" ]; then
    echo "Error: Serial port $SERIAL_PORT not found!"
    echo "Available ports:"
    ls -l /dev/tty.usbmodem* /dev/tty.SLAB* /dev/ttyUSB* 2>/dev/null || echo "  No USB serial devices found"
    exit 1
fi

# 烧录
esptool.py --chip esp32s3 --port "$SERIAL_PORT" --baud 921600 write_flash \
    0x0000 bootloader.bin \
    0x8000 partition-table.bin \
    0x10000 esp32-watch.bin \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 16MB

echo ""
echo "Flash complete! Resetting ESP32..."
sleep 1
echo "Done."
FLASH_SCRIPT

    chmod +x "$PACKAGE_DIR/flash.sh"
    
    # 创建 Windows 刷机脚本
    cat > "$PACKAGE_DIR/flash.bat" << 'FLASH_BAT'
@echo off
REM ESP32-S3 Flash Script for Windows
REM Usage: flash.bat [COM_PORT]
REM Default: COM3

set SERIAL_PORT=%1
if "%SERIAL_PORT%"=="" set SERIAL_PORT=COM3

echo Flashing ESP32-S3 Watch...
echo Serial port: %SERIAL_PORT%
echo.

esptool.py --chip esp32s3 --port %SERIAL_PORT% --baud 921600 write_flash ^
    0x0000 bootloader.bin ^
    0x8000 partition-table.bin ^
    0x10000 esp32-watch.bin ^
    --flash_mode dio ^
    --flash_freq 80m ^
    --flash_size 16MB

echo.
echo Flash complete!
pause
FLASH_BAT

    # 创建 README
    cat > "$PACKAGE_DIR/README.md" << README
# ESP32-S3 Watch Firmware

**Version:** $VERSION  
**Build Date:** $(date '+%Y-%m-%d %H:%M:%S')  
**Project:** $PROJECT_DIR

## Files

- \`bootloader.bin\` - ESP32-S3 bootloader (0x0000)
- \`partition-table.bin\` - Partition table (0x8000)
- \`esp32-watch.bin\` - Application (0x10000)

## Flash Instructions

### macOS / Linux

\`\`\`bash
# Using the flash script
./flash.sh [serial_port]

# Default port: /dev/tty.usbmodem14101
./flash.sh

# Custom port
./flash.sh /dev/tty.usbmodem12345

# Or manually with esptool
esptool.py --chip esp32s3 --port /dev/tty.usbmodem14101 write_flash \\
    0x0000 bootloader.bin \\
    0x8000 partition-table.bin \\
    0x10000 esp32-watch.bin
\`\`\`

### Windows

\`\`\`batch
REM Using the flash script
flash.bat [COM_PORT]

REM Default: COM3
flash.bat

REM Custom COM port
flash.bat COM5
\`\`\`

## Monitor Serial Output

\`\`\`bash
# macOS / Linux
idf.py -p /dev/tty.usbmodem14101 monitor

# Windows
idf.py -p COM3 monitor

# Or with screen (macOS/Linux)
screen /dev/tty.usbmodem14101 115200
\`\`\`

Press Ctrl+] then type 'quit' to exit screen.

## Features

- Event-driven architecture
- Power management (< 100uA deep sleep)
- LVGL GUI test menu
- Component tests: Display, Touch, IMU, Audio, Battery, BLE

## Support

Project: https://github.com/zhangfuwen/esp32-s3-watch
README

    # 创建压缩包
    cd "$PACKAGE_DIR"
    ARCHIVE_NAME="${PROJECT_NAME}-${VERSION}-$(date '+%Y%m%d-%H%M%S').zip"
    zip -r "$PROJECT_DIR/$ARCHIVE_NAME" .
    
    log_info "Package created: $PROJECT_DIR/$ARCHIVE_NAME"
    log_info "Package contents:"
    ls -lh "$PACKAGE_DIR"
}

# 通过 Telegram 发送
send_telegram() {
    if [ -z "$TELEGRAM_CHAT_ID" ] || [ -z "$TELEGRAM_BOT_TOKEN" ]; then
        log_warn "TELEGRAM_CHAT_ID or TELEGRAM_BOT_TOKEN not set. Skipping Telegram send."
        return 0
    fi
    
    log_info "Sending to Telegram..."
    
    ARCHIVE_PATH="$PROJECT_DIR/$ARCHIVE_NAME"
    
    # 发送文件
    curl -s -X POST "https://api.telegram.org/bot$TELEGRAM_BOT_TOKEN/sendDocument" \
        -F "chat_id=$TELEGRAM_CHAT_ID" \
        -F "document=@$ARCHIVE_PATH" \
        -F "caption=📦 ESP32-S3 Watch Firmware v$VERSION

🔧 Build Date: $(date '+%Y-%m-%d %H:%M:%S')
📁 Size: $(du -h \"$ARCHIVE_PATH\" | cut -f1)

📝 Contents:
• bootloader.bin
• partition-table.bin  
• esp32-watch.bin
• flash.sh / flash.bat
• README.md

💡 Usage: ./flash.sh [serial_port]" > /dev/null
    
    log_info "Sent to Telegram!"
}

# 显示帮助
show_help() {
    cat << HELP
ESP32-S3 Watch Build & Flash Script

Usage: $0 [command] [options]

Commands:
    all         Build, package, and send (default)
    build       Build only
    package     Package only (requires build)
    send        Send existing package to Telegram
    clean       Clean build artifacts
    flash       Flash to device
    help        Show this help

Options:
    -p, --port      Serial port (default: /dev/tty.usbmodem14101)
    -v, --version   Firmware version string
    -n, --no-send   Don't send to Telegram

Environment Variables:
    SERIAL_PORT         Default serial port
    TELEGRAM_CHAT_ID    Telegram chat ID for sending
    TELEGRAM_BOT_TOKEN  Telegram bot token
    VERSION             Firmware version

Examples:
    $0                      # Build, package, send
    $0 build                # Build only
    $0 package              # Package existing build
    $0 flash -p /dev/cu.usbmodem12345
    $0 send                 # Send to Telegram
    VERSION=v1.2.3 $0       # Custom version

HELP
}

# 烧录固件
flash() {
    log_info "Flashing to $SERIAL_PORT..."
    
    if [ ! -f "$PACKAGE_DIR/flash.sh" ]; then
        log_error "Package not found. Run '$0 package' first."
        exit 1
    fi
    
    "$PACKAGE_DIR/flash.sh" "$SERIAL_PORT"
}

# 主流程
main() {
    case "${1:-all}" in
        all)
            clean
            build
            package
            send_telegram
            log_info "All done! 🎉"
            ;;
        build)
            build
            ;;
        package)
            package
            ;;
        send)
            send_telegram
            ;;
        clean)
            clean
            log_info "Clean complete."
            ;;
        flash)
            flash
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            log_error "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
