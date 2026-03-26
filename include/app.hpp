#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <networktables/NetworkTableInstance.h>
#include <networktables/BooleanTopic.h>
#include <networktables/IntegerTopic.h>
#include <networktables/StructTopic.h>
#include <wpistruct/WPIStruct.h>

/**
 * @brief Represents a bounding box for a detection.
 */
struct BBox {
    int x, y, width, height;
};

/**
 * @brief Represents a detected object (ball or robot).
 */
struct Detection {
    int id;
    std::string label;
    float confidence;
    BBox bbox;

    WPISTRUCT_DECLARE_FIELDS
};

WPISTRUCT_DEFINE_FIELDS(Detection, id, label, confidence, bbox.x, bbox.y, bbox.width, bbox.height)

/**
 * @brief Configuration for the vision system.
 */
struct VisionConfig {
    int cameraIndex = 0;
    std::string modelPath = "models/yellow_ball.rknn";
    int nThreads = 1;
    bool debugDisplay = true;
    int teamNumber = 2026; // TODO: Change to your team number
};

class HopperDetector {
public:
    HopperDetector();
    ~HopperDetector();

    bool Initialize(const VisionConfig& config);
    void Run();

private:
    void ProcessFrame(cv::Mat& frame);
    void UpdateNetworkTables();

    VisionConfig config_;
    nt::NetworkTableInstance ntInst_;
    std::shared_ptr<nt::NetworkTable> table_;
    
    // Topics
    nt::BooleanPublisher ballPresentPub_;
    nt::IntegerPublisher ballCountPub_;
    nt::StructArrayPublisher<Detection> detectionsPub_;

    // Hopper State
    bool ballPresent_ = false;
    int ballCount_ = 0;
    std::vector<Detection> currentDetections_;

    // RKNN state would go here
};
