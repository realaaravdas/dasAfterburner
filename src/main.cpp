#include "app.hpp"
#include <iostream>
#include <chrono>
#include <thread>

// ── Constructor / Destructor ──────────────────────────────────────────────────

HopperDetector::HopperDetector()
#ifdef HAVE_NTCORE
    : ntInst_(nt::NetworkTableInstance::GetDefault())
#endif
{}

HopperDetector::~HopperDetector() {
#ifdef HAVE_RKNN
    if (rknnCtx_) {
        rknn_destroy(rknnCtx_);
    }
#endif
}

// ── Initialize ────────────────────────────────────────────────────────────────

bool HopperDetector::Initialize(const VisionConfig& config) {
    config_ = config;

#ifdef HAVE_NTCORE
    std::cout << "Starting NetworkTables client for team " << config_.teamNumber << std::endl;
    ntInst_.StartClient4("OrangePi_Vision");
    ntInst_.SetServerTeam(config_.teamNumber);
    ntInst_.StartDSClient();

    table_          = ntInst_.GetTable("Vision/Hopper");
    ballPresentPub_ = table_->GetBooleanTopic("BallPresent").Publish();
    ballCountPub_   = table_->GetIntegerTopic("BallCount").Publish();
    detectionsPub_  = table_->GetStructArrayTopic<Detection>("Detections").Publish();
#else
    std::cout << "NetworkTables disabled (build without HAVE_NTCORE)" << std::endl;
#endif

#ifdef HAVE_RKNN
    // Load RKNN model
    std::cout << "Loading model: " << config_.modelPath << std::endl;
    FILE* fp = fopen(config_.modelPath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Error: Cannot open model file: " << config_.modelPath << std::endl;
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
        std::cerr << "Error: rknn_init failed with code " << ret << std::endl;
        return false;
    }
    std::cout << "Model loaded OK." << std::endl;
#else
    std::cout << "RKNN NPU disabled (placeholder inference only)" << std::endl;
#endif

    return true;
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void HopperDetector::Run() {
    cv::VideoCapture cap(config_.cameraIndex);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera " << config_.cameraIndex << std::endl;
        return;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS,          30);

    std::cout << "Camera opened. Running vision loop..." << std::endl;

    cv::Mat frame;
    while (true) {
        if (!cap.read(frame)) {
            std::cerr << "Error: Failed to grab frame" << std::endl;
            break;
        }

        ProcessFrame(frame);
        UpdateNetworkTables();

        if (config_.debugDisplay) {
            for (const auto& det : currentDetections_) {
                cv::Rect rect(det.bbox.x, det.bbox.y, det.bbox.width, det.bbox.height);
                cv::rectangle(frame, rect, cv::Scalar(0, 255, 255), 2);
                cv::putText(frame, det.label + " " + std::to_string(det.confidence),
                            rect.tl(), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                            cv::Scalar(255, 255, 255), 1);
            }
            cv::imshow("Hopper Debug", frame);
            if (cv::waitKey(1) == 27) break; // ESC to quit
        }
    }
}

// ── ProcessFrame ──────────────────────────────────────────────────────────────

void HopperDetector::ProcessFrame(cv::Mat& frame) {
    currentDetections_.clear();

#ifdef HAVE_RKNN
    // TODO: implement RKNN inference
    // 1. Resize frame to model input size (typically 640×640 for YOLOv8)
    // 2. rknn_input inputs[1]; ... rknn_run(rknnCtx_, nullptr);
    // 3. Retrieve outputs, apply anchor decoding + NMS
    // 4. Populate currentDetections_
    (void)frame;
#else
    // Placeholder: simple HSV yellow-ball detector (no NPU)
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::Mat mask;
    cv::inRange(hsv, cv::Scalar(20, 100, 100), cv::Scalar(35, 255, 255), mask);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& c : contours) {
        if (cv::contourArea(c) < 500) continue; // filter noise
        cv::Rect r = cv::boundingRect(c);
        Detection det;
        det.id = static_cast<int>(currentDetections_.size());
        det.label = "yellow_ball";
        det.confidence = 0.0f; // HSV heuristic, not a real score
        det.bbox = {r.x, r.y, r.width, r.height};
        currentDetections_.push_back(det);
    }
#endif

    ballCount_   = static_cast<int>(currentDetections_.size());
    ballPresent_ = ballCount_ > 0;
}

// ── UpdateNetworkTables ───────────────────────────────────────────────────────

void HopperDetector::UpdateNetworkTables() {
#ifdef HAVE_NTCORE
    ballPresentPub_.Set(ballPresent_);
    ballCountPub_.Set(ballCount_);
    detectionsPub_.Set(currentDetections_);
#endif
    // When ntcore is disabled, detections are only visible in the debug window.
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    VisionConfig config;
    config.cameraIndex  = 0;
    config.modelPath    = "models/yellow_ball.rknn";
    config.debugDisplay = true;  // set false for headless/competition
    config.teamNumber   = 2026;

    HopperDetector detector;
    if (!detector.Initialize(config)) {
        std::cerr << "Initialization failed." << std::endl;
        return 1;
    }
    detector.Run();
    return 0;
}
