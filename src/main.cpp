#include "app.hpp"
#include <iostream>
#include <chrono>
#include <thread>

HopperDetector::HopperDetector() : ntInst_(nt::NetworkTableInstance::GetDefault()) {}

HopperDetector::~HopperDetector() {
    // Clean up RKNN and camera resources here
}

bool HopperDetector::Initialize(const VisionConfig& config) {
    config_ = config;

    // 1. Initialize NetworkTables
    std::cout << "Starting NetworkTables as client for team " << config_.teamNumber << std::endl;
    ntInst_.StartClient4("OrangePi_Vision");
    ntInst_.SetServerTeam(config_.teamNumber);
    ntInst_.StartDSClient(); // Also listen to DS for team number

    table_ = ntInst_.GetTable("Vision/Hopper");
    ballPresentPub_ = table_->GetBooleanTopic("BallPresent").Publish();
    ballCountPub_ = table_->GetIntegerTopic("BallCount").Publish();
    detectionsPub_ = table_->GetStructArrayTopic<Detection>("Detections").Publish();

    // 2. Initialize Camera
    // Note: On Linux, USB cameras are often /dev/videoX
    std::cout << "Initializing camera " << config_.cameraIndex << "..." << std::endl;
    // cv::VideoCapture cap(config_.cameraIndex);
    // if (!cap.isOpened()) {
    //     std::cerr << "Error: Could not open camera " << config_.cameraIndex << std::endl;
    //     return false;
    // }

    // 3. Initialize RKNN (NPU)
    // TODO: Load model from config_.modelPath using rknn_init
    // rknn_context ctx;
    // int ret = rknn_init(&ctx, model_data, model_size, 0, NULL);
    std::cout << "Loading model: " << config_.modelPath << " (NPU inference placeholder)" << std::endl;

    return true;
}

void HopperDetector::Run() {
    cv::VideoCapture cap(config_.cameraIndex);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera " << config_.cameraIndex << std::endl;
        return;
    }

    // Set camera resolution for performance
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS, 30);

    cv::Mat frame;
    while (true) {
        if (!cap.read(frame)) {
            std::cerr << "Error: Failed to grab frame" << std::endl;
            break;
        }

        ProcessFrame(frame);
        UpdateNetworkTables();

        if (config_.debugDisplay) {
            // Draw bounding boxes
            for (const auto& det : currentDetections_) {
                cv::Rect rect(det.bbox.x, det.bbox.y, det.bbox.width, det.bbox.height);
                cv::rectangle(frame, rect, cv::Scalar(0, 255, 255), 2);
                cv::putText(frame, det.label + " " + std::to_string(det.confidence), 
                            rect.tl(), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
            }

            cv::imshow("Hopper Debug", frame);
            if (cv::waitKey(1) == 27) break; // ESC to exit
        }
    }
}

void HopperDetector::ProcessFrame(cv::Mat& frame) {
    // 1. Pre-process (resize, convert color if needed for NPU)
    // 2. Run Inference (Placeholder)
    currentDetections_.clear();

    // Example Mock Detection:
    // This is where you would call rknn_run and post-process results.
    // For now, let's simulate a ball detection if we see something "yellowish" 
    // or just a placeholder for the user to fill in with their actual RKNN code.
    
    /*
    Detection mockBall;
    mockBall.id = 1;
    mockBall.label = "yellow_ball";
    mockBall.confidence = 0.95f;
    mockBall.bbox = cv::Rect(100, 100, 50, 50);
    currentDetections_.push_back(mockBall);
    */

    // Update hopper state based on detections
    ballCount_ = currentDetections_.size();
    ballPresent_ = ballCount_ > 0;
}

void HopperDetector::UpdateNetworkTables() {
    ballPresentPub_.Set(ballPresent_);
    ballCountPub_.Set(ballCount_);
    detectionsPub_.Set(currentDetections_);
}

int main() {
    VisionConfig config;
    config.cameraIndex = 0; // Adjust as needed
    config.modelPath = "models/yellow_ball.rknn";
    config.debugDisplay = true; // Set to false for headless production
    config.teamNumber = 2026;

    HopperDetector detector;
    if (detector.Initialize(config)) {
        detector.Run();
    }

    return 0;
}
