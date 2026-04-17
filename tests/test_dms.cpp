#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "../src/dms_monitor.h"

// Тест 1: Инициализация по умолчанию и пустые пути загрузки (система не падает)
TEST(DMSMonitorTest, ConstructorNoCrash) {
    EXPECT_NO_THROW({
        DMSMonitor monitor;
    });
}

// Тест 2: Вызов analyze() на пустом кадре возвращает face_detected = false, 
// и не падает из-за обращения к null матрицам
TEST(DMSMonitorTest, AnalyzeEmptyFrameReturnsFalse) {
    DMSMonitor monitor;
    cv::Mat empty_frame;
    
    DriverState state = monitor.analyze(empty_frame);
    
    EXPECT_FALSE(state.face_detected);
    EXPECT_FALSE(state.eyes_open);
}

// Тест 3: Вызов analyze() на залитом цветом кадре без лица
TEST(DMSMonitorTest, AnalyzeSolidColorFrameNoFace) {
    DMSMonitor monitor;
    cv::Mat solid_frame(480, 640, CV_8UC3, cv::Scalar(0, 255, 0)); // Зеленый экран
    
    DriverState state = monitor.analyze(solid_frame);
    
    EXPECT_FALSE(state.face_detected);
    // При отсуствии лица система должна переходить в состояние Distracted
    EXPECT_TRUE(state.alert_distracted); 
}
