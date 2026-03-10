#!/bin/bash
# Generate Chinese font using lv_font_conv
# Requires: npm install -g lv_font_conv

# Download Source Han Sans if not exists
if [ ! -f "SourceHanSansSC-Regular.otf" ]; then
    echo "Downloading Source Han Sans SC..."
    wget https://github.com/adobe-fonts/source-han-sans/raw/release/OTF/SimplifiedChinese/SourceHanSansSC-Regular.otf
fi

# Generate 16px Chinese font for watch display
# Range: Common watch characters (25 chars)
lv_font_conv --font SourceHanSansSC-Regular.otf \
    --size 16 \
    --format lvgl \
    --output main/fonts/chinese_16.c \
    --range 0x4e00-0x4e19 \
    --bpp 1

echo "Chinese font generated: main/fonts/chinese_16.c"
ls -lh main/fonts/chinese_16.c
