/**
 * @file dms_hud.h
 * @brief HUD (Head-Up Display) overlay for the Driver Monitoring System.
 */
#pragma once

#include <opencv2/opencv.hpp>
#include "dms_monitor.h"

/**
 * @brief Renders DMS status indicators and alerts on the camera frame.
 * 
 * Draws corner-style face tracking brackets, status panel
 * (eyes open/closed, head direction), drowsiness alert banner,
 * and distraction warning bar.
 */
class DMSHUD {
public:
    DMSHUD();

    /**
     * @brief Render the DMS overlay on a camera frame.
     * @param frame Camera frame to draw on (modified in place).
     * @param state Current driver state from DMSMonitor::analyze().
     */
    void render(cv::Mat& frame, const DriverState& state);
    
private:
    /**
     * @brief Draw corner-only rectangle brackets (HUD style).
     * @param frame Target frame.
     * @param rect Rectangle to draw corners for.
     * @param color Line color (BGR).
     * @param thickness Line thickness.
     * @param length Corner line length in pixels.
     */
    void drawCornerRect(cv::Mat& frame, cv::Rect rect, const cv::Scalar& color, int thickness, int length);
};
