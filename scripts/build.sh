#!/bin/bash
# build.sh

set -e

mkdir -p build
cd build

# Assuming ntcore and rknn_api are installed or their paths are provided
# cmake -Dntcore_DIR=/path/to/ntcore/lib/cmake ..
cmake ..

make -j$(nproc)

echo "Build complete. Binary is in build/dasAfterburner"
