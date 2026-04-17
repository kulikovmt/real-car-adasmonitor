/**
 * @file dms_monitor.h
 * @brief Driver Monitoring System (DMS) — face and eye detection module.
 */
#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <deque>

/**
 * @brief Current driver state as detected by the DMS.
 */
struct DriverState {
    bool face_detected = false;     ///< Whether a face was detected in the frame.
    bool eyes_open = true;          ///< Whether eyes are detected as open.
    bool looking_forward = true;    ///< Whether the driver is looking forward.
    float eye_openness = 1.0f;      ///< Eye openness ratio (0.0 closed — 1.0 open).
    float head_turn_deg = 0.0f;     ///< Estimated head turn angle in degrees.
    bool alert_drowsy = false;      ///< Drowsiness alert (eyes closed too long).
    bool alert_distracted = false;  ///< Distraction alert (head turned away).
    cv::Rect face_rect;             ///< Bounding rectangle of the detected face.
};

/**
 * @brief Analyzes webcam frames to detect driver fatigue and distraction.
 * 
 * Uses a DNN-based face detector (Caffe SSD) and a Haar Cascade eye
 * detector. Maintains a rolling history buffer of 30 frames to determine
 * sustained eye closure (drowsiness alert: eyes closed in 20 of 30 frames).
 * Head turn estimation is based on face center offset from frame center.
 */
class DMSMonitor {
public:
    DMSMonitor();

    /**
     * @brief Analyze a single video frame for driver state.
     * @param frame Input BGR frame from webcam.
     * @return DriverState with detection results and alerts.
     */
    DriverState analyze(const cv::Mat& frame);

private:
    cv::dnn::Net face_net_;              ///< DNN face detector (Caffe SSD).
    cv::CascadeClassifier eye_cascade_;  ///< Haar cascade eye detector.
    
    std::deque<bool> eye_history_;       ///< Rolling buffer of eye-open states (30 frames).
    
    int frame_counter_ = 0;             ///< Frame counter for DNN caching.
    cv::Rect last_face_rect_;           ///< Cached face rectangle.

    /**
     * @brief Detect the best face in the frame using DNN.
     * @param frame Input BGR frame.
     * @return Bounding rectangle of the best face (confidence > 0.5).
     */
    cv::Rect detectFace(const cv::Mat& frame);

    /**
     * @brief Estimate eye openness using Haar cascade in the upper face ROI.
     * @param frame Input BGR frame.
     * @param face_rect Detected face region.
     * @return 1.0 if eyes detected (open), 0.0 otherwise.
     */
    float estimateEyeOpenness(const cv::Mat& frame, cv::Rect face_rect);

    /**
     * @brief Estimate head turn angle from face center offset.
     * @param frame Input BGR frame.
     * @param face_rect Detected face region.
     * @return Estimated turn angle in degrees (-45 to +45).
     */
    float estimateHeadTurn(const cv::Mat& frame, cv::Rect face_rect);
};
