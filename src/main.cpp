#include "app.hpp"
#include <iostream>
#include <chrono>
#include <thread>

// ── Constructor / Destructor ──────────────────────────────────────────────────

HopperDetector::HopperDetector() {}

HopperDetector::~HopperDetector() {
    // Drive all outputs low before releasing
    if (ballPresentLine_) { gpiod_line_set_value(ballPresentLine_, 0); gpiod_line_release(ballPresentLine_); }
    if (countBit0Line_)   { gpiod_line_set_value(countBit0Line_,   0); gpiod_line_release(countBit0Line_);   }
    if (countBit1Line_)   { gpiod_line_set_value(countBit1Line_,   0); gpiod_line_release(countBit1Line_);   }
    if (countBit2Line_)   { gpiod_line_set_value(countBit2Line_,   0); gpiod_line_release(countBit2Line_);   }
    if (gpioChip_)        { gpiod_chip_close(gpioChip_); }

#ifdef HAVE_RKNN
    if (rknnCtx_) rknn_destroy(rknnCtx_);
#endif
}

// ── Initialize ────────────────────────────────────────────────────────────────

bool HopperDetector::Initialize(const VisionConfig& config, const GpioConfig& gpio) {
    config_ = config;

    // GPIO setup
    std::cout << "Opening GPIO chip: " << gpio.chipName << std::endl;
    gpioChip_ = gpiod_chip_open_by_name(gpio.chipName);
    if (!gpioChip_) {
        std::cerr << "GPIO: cannot open " << gpio.chipName
                  << " — check that libgpiod is installed and you have permission "
                     "(try running with sudo, or add user to 'gpio' group)" << std::endl;
        return false;
    }

    auto requestOutput = [&](unsigned lineNum, const char* label) -> gpiod_line* {
        gpiod_line* line = gpiod_chip_get_line(gpioChip_, lineNum);
        if (!line) {
            std::cerr << "GPIO: cannot get line " << lineNum << " (" << label << ")" << std::endl;
            return nullptr;
        }
        if (gpiod_line_request_output(line, "dasAfterburner", 0) < 0) {
            std::cerr << "GPIO: cannot request output on line " << lineNum
                      << " (" << label << ") — already in use?" << std::endl;
            return nullptr;
        }
        std::cout << "  GPIO line " << lineNum << " (" << label << ") ready" << std::endl;
        return line;
    };

    ballPresentLine_ = requestOutput(gpio.ballPresentLine, "BALL_PRESENT");
    countBit0Line_   = requestOutput(gpio.countBit0Line,   "COUNT_BIT0");
    countBit1Line_   = requestOutput(gpio.countBit1Line,   "COUNT_BIT1");
    countBit2Line_   = requestOutput(gpio.countBit2Line,   "COUNT_BIT2");

    if (!ballPresentLine_ || !countBit0Line_ || !countBit1Line_ || !countBit2Line_) {
        return false;
    }

#ifdef HAVE_RKNN
    std::cout << "Loading model: " << config_.modelPath << std::endl;
    FILE* fp = fopen(config_.modelPath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Error: cannot open model: " << config_.modelPath << std::endl;
        return false;
    }
    fseek(fp, 0, SEEK_END);
    size_t modelSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<uint8_t> modelData(modelSize);
    fread(modelData.data(), 1, modelSize, fp);
    fclose(fp);

    int ret = rknn_init(&rknnCtx_, modelData.data(), modelSize, 0, nullptr);
    if (ret != RKNN_SUCC) {
        std::cerr << "rknn_init failed: " << ret << std::endl;
        return false;
    }
    std::cout << "Model loaded." << std::endl;
#else
    std::cout << "RKNN disabled — using HSV fallback detector." << std::endl;
#endif

    return true;
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void HopperDetector::Run() {
    cv::VideoCapture cap(config_.cameraIndex);
    if (!cap.isOpened()) {
        std::cerr << "Error: cannot open camera " << config_.cameraIndex << std::endl;
        return;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS,          30);

    std::cout << "Running headless vision loop. Ctrl+C to stop." << std::endl;

    cv::Mat frame;
    int frameCount = 0;
    while (true) {
        if (!cap.read(frame)) {
            std::cerr << "Error: failed to grab frame" << std::endl;
            break;
        }

        ProcessFrame(frame);
        WriteGpio();
        ++frameCount;

        // Status every 5 s (~150 frames at 30 fps)
        if (frameCount % 150 == 0) {
            std::cout << "balls=" << ballCount_
                      << " present=" << ballPresent_ << std::endl;
        }
    }
}

// ── ProcessFrame ──────────────────────────────────────────────────────────────

void HopperDetector::ProcessFrame(cv::Mat& frame) {
    currentDetections_.clear();

#ifdef HAVE_RKNN
    // TODO: implement RKNN inference
    // 1. Resize to model input size (640×640 for YOLOv8)
    // 2. rknn_run(), retrieve outputs, apply NMS
    // 3. Populate currentDetections_
    (void)frame;
#else
    // HSV yellow-ball fallback
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::Mat mask;
    cv::inRange(hsv, cv::Scalar(20, 100, 100), cv::Scalar(35, 255, 255), mask);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& c : contours) {
        if (cv::contourArea(c) < 500) continue;
        cv::Rect r = cv::boundingRect(c);
        Detection det;
        det.id         = static_cast<int>(currentDetections_.size());
        det.label      = "yellow_ball";
        det.confidence = 0.0f;
        det.bbox       = {r.x, r.y, r.width, r.height};
        currentDetections_.push_back(det);
    }
#endif

    ballCount_   = static_cast<int>(currentDetections_.size());
    ballPresent_ = ballCount_ > 0;
}

// ── WriteGpio ─────────────────────────────────────────────────────────────────

void HopperDetector::WriteGpio() {
    // Clamp count to 3-bit range (0–7)
    int count = std::min(ballCount_, 7);

    gpiod_line_set_value(ballPresentLine_, ballPresent_ ? 1 : 0);
    gpiod_line_set_value(countBit0Line_,   (count >> 0) & 1);
    gpiod_line_set_value(countBit1Line_,   (count >> 1) & 1);
    gpiod_line_set_value(countBit2Line_,   (count >> 2) & 1);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    VisionConfig config;
    config.cameraIndex = 0;
    config.modelPath   = "models/yellow_ball.rknn";

    GpioConfig gpio;
    // Override defaults here if your wiring differs, e.g.:
    // gpio.ballPresentLine = 12;

    HopperDetector detector;
    if (!detector.Initialize(config, gpio)) {
        std::cerr << "Initialization failed." << std::endl;
        return 1;
    }
    detector.Run();
    return 0;
}
