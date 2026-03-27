# Installation Guide — Orange Pi 5 (Ubuntu 22.04 ARM64)

This guide covers getting dasAfterburner built and running on the Orange Pi 5
using the official OrangePi Ubuntu image from their Google Drive.

---

## Prerequisites

- Orange Pi 5 flashed with the OrangePi Ubuntu image (Ubuntu 22.04 Jammy, ARM64)
- Internet connection
- USB camera (or CSI camera via `/dev/videoX`)
- (Optional) RKNN model files in `models/`

The default login for the OrangePi image is:
```
user: orangepi
password: orangepi
```

---

## Quick Start

```bash
# 1. Clone the repo
cd ~/Desktop
git clone https://github.com/realaaravdas/dasAfterburner.git
cd dasAfterburner

# 2. Make scripts executable
chmod +x scripts/setup_orangepi.sh scripts/build.sh

# 3. Run setup (installs everything — takes a few minutes)
sudo ./scripts/setup_orangepi.sh

# 4. Build
./scripts/build.sh

# 5. Run
./build/dasAfterburner
```

---

## What the Setup Script Does

`scripts/setup_orangepi.sh` performs three steps:

### Step 1 — System packages
```bash
sudo apt-get install -y --allow-change-held-packages \
    build-essential cmake git curl unzip \
    libopencv-dev v4l-utils
```
The OrangePi image holds some packages (like `v4l-utils`) at custom versions.
`--allow-change-held-packages` is required to upgrade them.

### Step 2 — RKNN NPU SDK
Clones `https://github.com/rockchip-linux/rknpu2` into `third_party/rknpu2/`
and copies `librknnrt.so` and `rknn_api.h` to `/usr/local/`.

### Step 3 — NetworkTables (ntcore)
Downloads the latest WPILib ARM64 release from GitHub and installs:
- `wpiutil` (required by ntcore)
- `wpinet`  (required by ntcore)
- `ntcore`
- `wpistruct` (struct serialisation)

---

## Build Options

The build system auto-detects available libraries and disables missing ones
gracefully. You can also force options:

```bash
# Full build (default — requires ntcore + RKNN)
./scripts/build.sh

# Build without NetworkTables (useful for standalone testing)
./scripts/build.sh -DENABLE_NTCORE=OFF

# Build without NPU (uses HSV yellow-ball fallback)
./scripts/build.sh -DENABLE_RKNN=OFF

# Minimal build — no NT, no NPU (camera + HSV only)
./scripts/build.sh -DENABLE_NTCORE=OFF -DENABLE_RKNN=OFF
```

When built without ntcore, detections are shown only in the debug window.
When built without RKNN, the code falls back to a simple HSV colour filter.

---

## Manual Dependency Install (if the script fails)

### OpenCV
```bash
sudo apt-get install -y libopencv-dev
```

### RKNN SDK (manual)
```bash
git clone https://github.com/rockchip-linux/rknpu2.git --depth 1
sudo cp rknpu2/runtime/RK3588/Linux/librknnrt/aarch64/librknnrt.so /usr/local/lib/
sudo cp rknpu2/runtime/RK3588/Linux/librknnrt/include/rknn_api.h /usr/local/include/
sudo ldconfig
```

### ntcore / WPILib (manual)
1. Find the latest version number at:
   `https://frcmaven.wpi.edu/release/edu/wpi/first/ntcore/ntcore-cpp/maven-metadata.xml`
   (look for `<latest>`)
2. Download the following ARM64 zips (replace `VERSION` with that version, e.g. `2026.1.1`):
   Base URL: `https://frcmaven.wpi.edu/release/edu/wpi/first/{component}/{component}-cpp/{VERSION}/{component}-cpp-{VERSION}-linuxarm64.zip`
   - `wpiutil-cpp-VERSION-linuxarm64.zip`
   - `wpinet-cpp-VERSION-linuxarm64.zip`
   - `ntcore-cpp-VERSION-linuxarm64.zip`
   - `wpistruct-cpp-VERSION-linuxarm64.zip`
3. For each zip:
   ```bash
   unzip <name>.zip -d <name>
   sudo cp -r <name>/headers/. /usr/local/include/
   sudo find <name> -name "*.so*" -exec cp {} /usr/local/lib/ \;
   ```
4. Refresh the linker cache:
   ```bash
   sudo ldconfig
   ```

---

## Models

Place your `.rknn` model files in the `models/` directory:

```
models/
  yellow_ball.rknn   ← main ball detection model
  robots.rknn        ← (future) robot detection model
```

The model currently included (`Fuel-640-640-yolov8.rknn`) must be converted
to the filename expected by the config (`yellow_ball.rknn`) or update
`config.modelPath` in `src/main.cpp`.

---

## Running at Boot (systemd)

```bash
sudo cp config/vision.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable vision.service
sudo systemctl start vision.service
sudo systemctl status vision.service
```

For competition (headless), set `config.debugDisplay = false` in `src/main.cpp`
before building and make sure no display server is required.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `Permission denied` on script | `chmod +x scripts/*.sh` |
| `sudo: command not found` after `sudo ./script.sh` | Run as `bash script.sh` or `sudo bash script.sh` if `PATH` is missing |
| `cmake .. ` targets wrong dir | Always use `./scripts/build.sh` from the project root, never `cd scripts && ./build.sh` |
| `v4l-utils` held-package error | Script now uses `--allow-change-held-packages`; if it still fails, skip with `sudo apt-mark unhold v4l-utils` |
| `ntcore not found` at cmake | Run setup script, or pass `-DENABLE_NTCORE=OFF` to build without it |
| `rknn_init failed` | Check model path exists and is a valid RK3588 RKNN model |
| Camera not opening | Check `ls /dev/video*` and set `config.cameraIndex` accordingly |
| NetworkTables not connecting | Confirm RoboRIO IP/hostname reachable; check `config.teamNumber` matches your team |

---

## Network / RoboRIO Connection

The program resolves the RoboRIO address from the team number (FRC default):
- `roboRIO-2026-FRC.local` (mDNS)
- `10.20.26.2` (static fallback)

Connect the Orange Pi to the robot radio via Ethernet for competition.
For bench testing, connect both to the same switch or directly.
