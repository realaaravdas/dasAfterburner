#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <gpiod.h>

#ifdef HAVE_RKNN
#include <rknn_api.h>
#endif

struct BBox {
    int x, y, width, height;
};

struct Detection {
    int id;
    std::string label;
    float confidence;
    BBox bbox;
};

struct VisionConfig {
    int cameraIndex       = 0;
    std::string modelPath = "models/yellow_ball.rknn";
    int nThreads          = 1;
};

/**
 * GPIO output pin mapping for the Orange Pi 5 40-pin header.
 *
 * Verify pin names on your board:
 *   gpioinfo gpiochip1
 *
 * Physical → GPIO name   → chip/line used here
 * Pin 11   → GPIO1_B4    → gpiochip1 line 12   BALL_PRESENT
 * Pin 13   → GPIO1_B6    → gpiochip1 line 14   COUNT_BIT0 (LSB)
 * Pin 15   → GPIO1_B7    → gpiochip1 line 15   COUNT_BIT1
 * Pin 16   → GPIO1_C0    → gpiochip1 line 16   COUNT_BIT2 (MSB)
 *
 * 3-bit count encodes 0–7 balls. BALL_PRESENT mirrors (count > 0).
 * Use any GND pin (6, 9, 14, 20, 25) as common ground with the RoboRIO.
 */
struct GpioConfig {
    const char* chipName      = "gpiochip1";
    unsigned ballPresentLine  = 12; // physical pin 11
    unsigned countBit0Line    = 14; // physical pin 13
    unsigned countBit1Line    = 15; // physical pin 15
    unsigned countBit2Line    = 16; // physical pin 16
};

class HopperDetector {
public:
    HopperDetector();
    ~HopperDetector();

    bool Initialize(const VisionConfig& config, const GpioConfig& gpio = GpioConfig{});
    void Run();

private:
    void ProcessFrame(cv::Mat& frame);
    void WriteGpio();

    VisionConfig config_;

    gpiod_chip* gpioChip_        = nullptr;
    gpiod_line* ballPresentLine_ = nullptr;
    gpiod_line* countBit0Line_   = nullptr;
    gpiod_line* countBit1Line_   = nullptr;
    gpiod_line* countBit2Line_   = nullptr;

#ifdef HAVE_RKNN
    rknn_context rknnCtx_ = 0;
#endif

    bool ballPresent_ = false;
    int  ballCount_   = 0;
    std::vector<Detection> currentDetections_;
};
