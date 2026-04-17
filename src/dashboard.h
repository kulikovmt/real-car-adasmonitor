#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include "obd_parser.h"

class Dashboard {
public:
    Dashboard();
    ~Dashboard() = default;

    // Главный метод отрисовки 
    void draw(cv::Mat& frame, double speed, double rpm, double coolant, double fuel, double throttle, BehaviorStyle style);

private:
    // Ширина панели (половина фрейма 640x480)
    int panel_width_ = 320; 

    // Вспомогательные методы
    void drawGauge(cv::Mat& frame, cv::Point center, int radius, double value, double max_value, 
                   const std::string& label, const std::string& unit, bool is_red_zone);
                   
    void drawLinearGauge(cv::Mat& frame, cv::Rect rect, double value, double max_value, 
                         const std::string& label, const cv::Scalar& color);
                         
    void drawStyleText(cv::Mat& frame, cv::Point pos, BehaviorStyle style);
    void drawWarnings(cv::Mat& frame, cv::Point pos, double coolant, double fuel);
};
