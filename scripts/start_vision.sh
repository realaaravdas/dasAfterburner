#!/bin/bash
# start_vision.sh — Start dasAfterburner (used by systemd service)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_ROOT/build/dasAfterburner"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found: $BINARY"
    echo "Run ./scripts/build.sh first."
    exit 1
fi

exec "$BINARY"
