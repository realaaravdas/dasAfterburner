#!/bin/bash
# build.sh — Build dasAfterburner from project root
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

mkdir -p "$PROJECT_ROOT/build"
cd "$PROJECT_ROOT/build"

# Pass any extra cmake args through (e.g. -DENABLE_NTCORE=OFF)
cmake "$PROJECT_ROOT" "$@"

make -j$(nproc)

echo ""
echo "Build complete. Binary: $PROJECT_ROOT/build/dasAfterburner"
