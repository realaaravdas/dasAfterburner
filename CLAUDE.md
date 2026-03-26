# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

dasAfterburner is a C++17 vision system for FRC Team 2026, targeting the Orange Pi 5 (Rockchip RK3588 ARM64). It detects yellow balls using the RK3588 NPU (RKNN) and publishes results to the RoboRIO via NetworkTables (NT4).

## Build Commands

```bash
# First-time setup (Orange Pi 5 only)
chmod +x scripts/setup_orangepi.sh scripts/build.sh
./scripts/setup_orangepi.sh

# Build
./scripts/build.sh
# Output: build/dasAfterburner

# Run
./build/dasAfterburner

# Autostart via systemd
sudo cp config/vision.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable vision.service
sudo systemctl start vision.service
sudo systemctl status vision.service
```

No automated test suite exists. Testing requires real hardware (camera, RoboRIO, and `.rknn` model files in `models/`).

## Architecture

The pipeline is: **Camera (V4L2/OpenCV)** → **RKNN NPU inference** → **NetworkTables publish**.

### Key Files

- `include/app.hpp` — All data structures (`BBox`, `Detection`, `VisionConfig`) and the `HopperDetector` class declaration. `Detection` uses `WPISTRUCT` macros for NT4 serialization.
- `src/main.cpp` — `HopperDetector` method implementations and `main()`.

### HopperDetector Lifecycle

1. `Initialize()` — connects to RoboRIO via NT4, creates topic publishers, sets up camera and RKNN model (camera/RKNN currently placeholder).
2. `Run()` — main loop: captures 640×480@30fps frames, calls `ProcessFrame()`, calls `UpdateNetworkTables()`, optionally renders debug window.
3. `ProcessFrame()` — **placeholder** for `rknn_run` call + NMS post-processing. Must be implemented with actual RKNN API calls.
4. `UpdateNetworkTables()` — publishes to `Vision/Hopper` table: `BallPresent` (bool), `BallCount` (int), `Detections` (struct array).

### NetworkTables

- Client name: `"OrangePi_Vision"`, server: RoboRIO resolved from team number 2026.
- Table: `Vision/Hopper`
- `Detection` struct is published as a `WPIStruct` array — the RoboRIO-side code must match.

### Dependencies

| Library | Source | Notes |
|---|---|---|
| OpenCV | `apt` (`libopencv-dev`) | Camera capture + debug drawing |
| RKNN Runtime | Rockchip GitHub (`librknnrt.so`, `rknn_api.h`) | NPU inference |
| ntcore / WPILib | Manual ARM64 install | NetworkTables client |

RKNN and ntcore are currently **commented out** in `CMakeLists.txt` — re-enable them once libraries are installed on the target device.

## Active TODOs (from GEMINI.md)

- Implement RKNN post-processing in `ProcessFrame()` (anchor boxes, NMS).
- Add second camera + robot detection model (`models/robots.rknn`).
- Fine-tune camera exposure/gain for competition lighting.
- Correctly link `ntcore` static libraries in `CMakeLists.txt`.

## Competition Notes

- Set `config.debugDisplay = false` for headless/competition mode.
- Models go in `models/` (`yellow_ball.rknn`, `robots.rknn`).
- The systemd service (`config/vision.service`) runs as root and restarts every 5s on failure.
