#!/bin/bash
# setup_orangepi.sh — One-time setup for dasAfterburner on Orange Pi 5 (Ubuntu 22.04 ARM64)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
THIRD_PARTY="$PROJECT_ROOT/third_party"

# Must run as root
if [ "$EUID" -ne 0 ]; then
    echo "Run as root: sudo $0"
    exit 1
fi

echo "=== dasAfterburner Setup: Orange Pi 5 / Ubuntu 22.04 ARM64 ==="

# ── 1. System packages ──────────────────────────────────────────────────────
echo ""
echo "[1/3] Installing system dependencies..."
apt-get update
apt-get install -y --allow-change-held-packages \
    build-essential \
    cmake \
    git \
    curl \
    unzip \
    libopencv-dev \
    libgpiod-dev \
    v4l-utils

# ── 2. RKNN NPU SDK ─────────────────────────────────────────────────────────
echo ""
echo "[2/3] Installing RKNN NPU SDK (RK3588)..."
mkdir -p "$THIRD_PARTY"
RKNPU_DIR="$THIRD_PARTY/rknpu2"

if [ ! -d "$RKNPU_DIR" ]; then
    git clone https://github.com/rockchip-linux/rknpu2.git --depth 1 "$RKNPU_DIR"
else
    echo "  rknpu2 already cloned, skipping."
fi

RKNN_LIB="$RKNPU_DIR/runtime/RK3588/Linux/librknnrt/aarch64/librknnrt.so"
RKNN_HDR="$RKNPU_DIR/runtime/RK3588/Linux/librknnrt/include/rknn_api.h"

if [ -f "$RKNN_LIB" ] && [ -f "$RKNN_HDR" ]; then
    cp "$RKNN_LIB" /usr/local/lib/
    cp "$RKNN_HDR" /usr/local/include/
    ldconfig
    echo "  RKNN SDK installed."
else
    echo "  WARNING: RKNN library or header not found at expected paths."
    echo "  Expected: $RKNN_LIB"
    echo "  The build will proceed without NPU support (ENABLE_RKNN=OFF)."
fi

# ── 3. NetworkTables / ntcore (WPILib ARM64) ─────────────────────────────────
# Artifacts are on WPILib's Maven repo (frcmaven.wpi.edu), NOT GitHub releases.
# URL pattern:
#   https://frcmaven.wpi.edu/release/edu/wpi/first/<comp>/<comp>-cpp/<ver>/<comp>-cpp-<ver>-linuxarm64.zip
echo ""
echo "[3/3] Installing ntcore (WPILib NetworkTables) for ARM64..."

MAVEN_BASE="https://frcmaven.wpi.edu/release/edu/wpi/first"

# Fetch latest ntcore version from Maven metadata
WPILIB_VERSION=$(curl -sf \
    "${MAVEN_BASE}/ntcore/ntcore-cpp/maven-metadata.xml" \
    | grep -oP '(?<=<latest>)[^<]+' | head -1) || true

if [ -z "$WPILIB_VERSION" ]; then
    # Fallback: try to parse from allwpilib GitHub tag
    WPILIB_VERSION=$(curl -sf \
        "https://api.github.com/repos/wpilibsuite/allwpilib/releases/latest" \
        | grep '"tag_name"' | sed -E 's/.*"v([^"]+)".*/\1/') || true
fi

if [ -z "$WPILIB_VERSION" ]; then
    echo "  WARNING: Could not determine WPILib version."
    echo "  Install ntcore manually — see INSTALL.md for instructions."
else
    echo "  WPILib version: $WPILIB_VERSION"
    TMPDIR_WPI=$(mktemp -d)

    install_wpi_artifact() {
        local NAME="$1"
        local URL="${MAVEN_BASE}/${NAME}/${NAME}-cpp/${WPILIB_VERSION}/${NAME}-cpp-${WPILIB_VERSION}-linuxarm64.zip"
        echo "  Downloading $NAME from frcmaven..."
        if curl -fL --progress-bar "$URL" -o "$TMPDIR_WPI/$NAME.zip"; then
            unzip -qo "$TMPDIR_WPI/$NAME.zip" -d "$TMPDIR_WPI/$NAME"
            if [ -d "$TMPDIR_WPI/$NAME/headers" ]; then
                cp -r "$TMPDIR_WPI/$NAME/headers/." /usr/local/include/
            fi
            find "$TMPDIR_WPI/$NAME" -name "*.so*" -exec cp {} /usr/local/lib/ \;
            echo "    $NAME installed."
        else
            echo "    WARNING: Could not download $NAME"
            echo "    URL tried: $URL"
        fi
    }

    install_wpi_artifact "wpiutil"
    install_wpi_artifact "wpinet"
    install_wpi_artifact "ntcore"

    # wpistruct: separate artifact in some WPILib versions; bundled in wpiutil in others.
    # Try to download it, but don't fail if it's absent — wpiutil already covers it.
    WPISTRUCT_URL="${MAVEN_BASE}/wpistruct/wpistruct-cpp/${WPILIB_VERSION}/wpistruct-cpp-${WPILIB_VERSION}-linuxarm64.zip"
    echo "  Downloading wpistruct from frcmaven (optional)..."
    if curl -fL --progress-bar "$WPISTRUCT_URL" -o "$TMPDIR_WPI/wpistruct.zip" 2>/dev/null; then
        unzip -qo "$TMPDIR_WPI/wpistruct.zip" -d "$TMPDIR_WPI/wpistruct"
        if [ -d "$TMPDIR_WPI/wpistruct/headers" ]; then
            cp -r "$TMPDIR_WPI/wpistruct/headers/." /usr/local/include/
        fi
        find "$TMPDIR_WPI/wpistruct" -name "*.so*" -exec cp {} /usr/local/lib/ \;
        echo "    wpistruct installed."
    else
        echo "    wpistruct not available as a separate artifact (bundled in wpiutil — OK)."
    fi

    # Verify the header we actually need is present
    if [ -f /usr/local/include/wpistruct/WPIStruct.h ]; then
        echo "  wpistruct/WPIStruct.h found — OK"
    else
        echo "  WARNING: wpistruct/WPIStruct.h not found after install."
        echo "  HAVE_NTCORE will be disabled at build time."
    fi

    ldconfig
    rm -rf "$TMPDIR_WPI"
fi

# ── Done ─────────────────────────────────────────────────────────────────────
echo ""
echo "=== Setup complete! ==="
echo ""
echo "Next steps:"
echo "  1. Place your RKNN model in: $PROJECT_ROOT/models/yellow_ball.rknn"
echo "  2. Build:  cd $PROJECT_ROOT && sudo chmod +x scripts/build.sh && ./scripts/build.sh"
echo "  3. Run:    $PROJECT_ROOT/build/dasAfterburner"
echo ""
echo "To build without ntcore/RKNN (for testing):"
echo "  ./scripts/build.sh -DENABLE_NTCORE=OFF -DENABLE_RKNN=OFF"
