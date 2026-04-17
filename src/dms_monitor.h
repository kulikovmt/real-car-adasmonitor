#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <deque>

struct DriverState {
    bool face_detected = false;
    bool eyes_open = true;
    bool looking_forward = true;
    float eye_openness = 1.0f; // 0.0 - 1.0
    float head_turn_deg = 0.0f;
    bool alert_drowsy = false;
    bool alert_distracted = false;
    cv::Rect face_rect;
};

class DMSMonitor {
public:
    DMSMonitor();
    DriverState analyze(const cv::Mat& frame);

private:
    cv::dnn::Net face_net_;
    cv::CascadeClassifier eye_cascade_;
    
    std::deque<bool> eye_history_; // История открытия глаз (теперь 30 кадров)
    
    int frame_counter_ = 0;
    cv::Rect last_face_rect_;

    cv::Rect detectFace(const cv::Mat& frame);
    float estimateEyeOpenness(const cv::Mat& frame, cv::Rect face_rect);
    float estimateHeadTurn(const cv::Mat& frame, cv::Rect face_rect);
};
