# FRC 2026: dasAfterburner Vision System

**Goal:** High-performance vision system for Orange Pi 5 (RK3588) to detect yellow balls and robots for FRC 2026.

## Project Overview

- **Language:** C++17 (chosen for direct access to RKNN and FRC libraries).
- **Core Technologies:**
  - **Rockchip NPU (RKNN):** Uses the 6 TOPS NPU on the RK3588 for sub-ms inference.
  - **OpenCV:** For camera handling (V4L2) and drawing debug bounding boxes.
  - **NetworkTables (NT4):** For communication with the RoboRIO.
  - **WPIStruct:** For publishing complex detection objects.

## Directory Structure

- `src/`: Core C++ source files.
- `include/`: Header files and data structure definitions.
- `models/`: Place your `.rknn` models here.
- `scripts/`: Setup, build, and deployment scripts.
- `config/`: Systemd service files for autostart.

## Setup Instructions (Orange Pi 5)

### 1. Initial OS Setup
Ensure you're running Ubuntu 22.04 or Orange Pi OS.

### 2. Install Dependencies
Run the provided setup script:
```bash
chmod +x scripts/setup_orangepi.sh scripts/build.sh
./scripts/setup_orangepi.sh
```

### 3. Model Placement
Place your `.rknn` models in the `models/` directory:
- `models/yellow_ball.rknn`
- `models/robots.rknn`

### 4. Build the Project
```bash
./scripts/build.sh
```

### 5. Configure Autostart
To ensure the vision system starts on boot:
```bash
sudo cp config/vision.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable vision.service
sudo systemctl start vision.service
```

## NetworkTables Configuration

The system connects to the RoboRIO (Team 2026) by default. Data is published to the `Vision/Hopper` table:
- `BallPresent`: Boolean (Is there at least one ball?)
- `BallCount`: Integer (How many balls?)
- `Detections`: StructArray of `Detection` (Contains `id`, `label`, `confidence`, and `bbox`).

## Development Conventions

- **Performance:** Headless mode is recommended for competition (`config.debugDisplay = false`).
- **NPU Integration:** The `ProcessFrame` function in `src/main.cpp` is a placeholder. You must integrate the `rknn_run` calls and post-processing (NMS, etc.) specific to your model.
- **WPIStruct:** Used to publish the `Detection` struct. Ensure your RoboRIO code is updated to receive this struct array.

## To-Do
- [ ] Implement full RKNN post-processing (anchor boxes, NMS).
- [ ] Add support for the robot detection model (second camera).
- [ ] Fine-tune camera parameters (exposure, gain) for competition lighting.
- [ ] Ensure `ntcore` static libraries are correctly linked in `CMakeLists.txt`.
