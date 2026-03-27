#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

#ifdef HAVE_NTCORE
#include <networktables/NetworkTableInstance.h>
#include <networktables/BooleanTopic.h>
#include <networktables/IntegerTopic.h>
#include <networktables/StructTopic.h>
#include <wpistruct/WPIStruct.h>
#endif

#ifdef HAVE_RKNN
#include <rknn_api.h>
#endif

/**
 * @brief Bounding box for a detected object (pixel coordinates).
 */
struct BBox {
    int x, y, width, height;
};

/**
 * @brief A single detection result from the NPU model.
 *
 * When HAVE_NTCORE is defined the struct is annotated for WPIStruct
 * serialisation so it can be published as a typed array on NetworkTables.
 */
struct Detection {
    int id;
    std::string label;
    float confidence;
    BBox bbox;

#ifdef HAVE_NTCORE
    WPISTRUCT_DECLARE_FIELDS
#endif
};

#ifdef HAVE_NTCORE
WPISTRUCT_DEFINE_FIELDS(Detection,
    id, label, confidence,
    bbox.x, bbox.y, bbox.width, bbox.height)
#endif

/**
 * @brief Runtime configuration for the vision pipeline.
 */
struct VisionConfig {
    int cameraIndex    = 0;
    std::string modelPath = "models/yellow_ball.rknn";
    int nThreads       = 1;
    bool debugDisplay  = true;
    int teamNumber     = 2026;
};

/**
 * @brief Top-level class that owns the camera, NPU context, and NT publishers.
 *
 * Lifecycle: construct → Initialize() → Run() (blocks until camera error or ESC).
 */
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

#ifdef HAVE_NTCORE
    nt::NetworkTableInstance ntInst_;
    std::shared_ptr<nt::NetworkTable> table_;
    nt::BooleanPublisher              ballPresentPub_;
    nt::IntegerPublisher              ballCountPub_;
    nt::StructArrayPublisher<Detection> detectionsPub_;
#endif

#ifdef HAVE_RKNN
    rknn_context rknnCtx_ = 0;
#endif

    bool ballPresent_ = false;
    int  ballCount_   = 0;
    std::vector<Detection> currentDetections_;
};
