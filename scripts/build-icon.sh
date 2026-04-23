#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SVG="$PROJECT_DIR/assets/AppIcon.svg"
ICONSET="$PROJECT_DIR/assets/AppIcon.iconset"
ICNS="$PROJECT_DIR/assets/AppIcon.icns"

mkdir -p "$ICONSET"

export_png() {
    local size=$1
    local out=$2
    if command -v rsvg-convert &>/dev/null; then
        rsvg-convert -w "$size" -h "$size" "$SVG" -o "$out"
    else
        local tmp="$ICONSET/tmp_1024.png"
        if [ ! -f "$tmp" ]; then
            qlmanage -t -s 1024 -o "$ICONSET" "$SVG" 2>/dev/null
            mv "$ICONSET/AppIcon.svg.png" "$tmp" 2>/dev/null || true
        fi
        sips -z "$size" "$size" "$tmp" --out "$out" &>/dev/null
    fi
}

export_png 16    "$ICONSET/icon_16x16.png"
export_png 32    "$ICONSET/icon_16x16@2x.png"
export_png 32    "$ICONSET/icon_32x32.png"
export_png 64    "$ICONSET/icon_32x32@2x.png"
export_png 128   "$ICONSET/icon_128x128.png"
export_png 256   "$ICONSET/icon_128x128@2x.png"
export_png 256   "$ICONSET/icon_256x256.png"
export_png 512   "$ICONSET/icon_256x256@2x.png"
export_png 512   "$ICONSET/icon_512x512.png"
export_png 1024  "$ICONSET/icon_512x512@2x.png"

rm -f "$ICONSET/tmp_1024.png"
iconutil -c icns "$ICONSET" -o "$ICNS"
echo "Built: $ICNS"

# Build Windows .ico (16, 32, 48, 256) if convert (ImageMagick) is available
ICO="$PROJECT_DIR/assets/AppIcon.ico"
if command -v convert &>/dev/null; then
    convert \
        "$ICONSET/icon_16x16.png" \
        "$ICONSET/icon_32x32.png" \
        "$ICONSET/icon_32x32@2x.png" \
        "$ICONSET/icon_256x256.png" \
        "$ICO"
    echo "Built: $ICO"
fi
