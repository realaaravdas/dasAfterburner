#!/bin/bash
# setup_orangepi.sh

set -e

echo "Updating system and installing dependencies..."
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libopencv-dev \
    libpthread-stubs0-dev \
    v4l-utils

# RKNN NPU SDK Setup (Download from Rockchip)
echo "Setting up RKNN NPU SDK..."
if [ ! -d "rknpu2" ]; then
    git clone https://github.com/rockchip-linux/rknpu2.git --depth 1
fi
# Copy libraries and headers to system locations (or your project)
sudo cp rknpu2/runtime/RK3588/Linux/librknnrt/aarch64/librknnrt.so /usr/lib/
sudo cp rknpu2/runtime/RK3588/Linux/librknnrt/include/rknn_api.h /usr/include/

# NetworkTables (ntcore) and WPILib Setup
# For FRC, the easiest way is often to download prebuilt ARM64 binaries
# but we'll leave it as a TODO for the user to find their preferred source.
echo "NetworkTables (ntcore) needs to be installed for ARM64."
echo "Consider using: https://github.com/wpilibsuite/allwpilib/releases"

echo "Setup complete. Don't forget to put your .rknn models in the models/ directory."
