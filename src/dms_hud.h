#pragma once

#include <opencv2/opencv.hpp>
#include "dms_monitor.h"

class DMSHUD {
public:
    DMSHUD();
    // Отрисовывает интерфейс DMS (рамку лица, статусы глаз/головы, предупреждения)
    void render(cv::Mat& frame, const DriverState& state);
    
private:
    void drawCornerRect(cv::Mat& frame, cv::Rect rect, const cv::Scalar& color, int thickness, int length);
};
