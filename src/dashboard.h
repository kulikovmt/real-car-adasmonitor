/**
 * @file dashboard.h
 * @brief Premium automotive dashboard visualization module.
 */
#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include "obd_parser.h"

/**
 * @brief Renders a premium-styled automotive dashboard overlay.
 * 
 * Draws circular gauges (speedometer, tachometer), linear gauges
 * (coolant temperature, fuel level, throttle), driving style indicator,
 * and warning messages using OpenCV drawing primitives with
 * anti-aliasing and neon glow effects.
 */
class Dashboard {
public:
    Dashboard();
    ~Dashboard() = default;

    /**
     * @brief Main rendering method — draws all dashboard elements.
     * @param frame Target OpenCV Mat to draw on (modified in place).
     * @param speed Current speed in km/h.
     * @param rpm Engine RPM.
     * @param coolant Coolant temperature in Celsius.
     * @param fuel Fuel level in percent (0-100).
     * @param throttle Throttle position in percent (0-100).
     * @param style Current driving behavior classification.
     */
    void draw(cv::Mat& frame, double speed, double rpm, double coolant, double fuel, double throttle, BehaviorStyle style);

private:
    int panel_width_ = 320; ///< Dashboard panel width in pixels.

    /** @brief Draw a circular gauge (speedometer or tachometer). */
    void drawGauge(cv::Mat& frame, cv::Point center, int radius, double value, double max_value, 
                   const std::string& label, const std::string& unit, bool is_red_zone);
                   
    /** @brief Draw a horizontal linear gauge bar. */
    void drawLinearGauge(cv::Mat& frame, cv::Rect rect, double value, double max_value, 
                         const std::string& label, const cv::Scalar& color);
                         
    /** @brief Draw driving style text with color coding. */
    void drawStyleText(cv::Mat& frame, cv::Point pos, BehaviorStyle style);

    /** @brief Draw warning messages (overheat, low fuel). */
    void drawWarnings(cv::Mat& frame, cv::Point pos, double coolant, double fuel);
};
