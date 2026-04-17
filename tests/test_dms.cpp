#include <gtest/gtest.h>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "../src/dms_monitor.h"

// Тест 1: По умолчанию модуль не загружен (конструктор не падает)
TEST(DMSMonitorTest, DefaultModuleNotLoaded) {
    EXPECT_NO_THROW({
        DMSMonitor monitor;
    });
}

// Тест 2: analyze() на пустом кадре не падает, возвращает face_detected = false
TEST(DMSMonitorTest, AnalyzeEmptyFrameReturnsFalse) {
    DMSMonitor monitor;
    cv::Mat empty_frame;
    
    DriverState state = monitor.analyze(empty_frame);
    
    EXPECT_FALSE(state.face_detected);
    EXPECT_FALSE(state.eyes_open);
}

// Тест 3: Загрузка моделей (если файлы есть)
TEST(DMSMonitorTest, ModelsLoadedIfFilesExist) {
    // Проверяем что файлы моделей существуют на диске
    std::ifstream prototxt("models/deploy.prototxt");
    std::ifstream caffemodel("models/res10_300x300_ssd_iter_140000.caffemodel");
    std::ifstream haarcascade("models/haarcascade_eye.xml");
    
    bool files_exist = prototxt.good() && caffemodel.good() && haarcascade.good();
    
    if (files_exist) {
        // Если файлы на месте, модуль должен загрузиться без ошибок
        // и корректно анализировать кадр без лица (не крашиться)
        DMSMonitor monitor;
        cv::Mat test_frame(480, 640, CV_8UC3, cv::Scalar(128, 128, 128));
        
        DriverState state = monitor.analyze(test_frame);
        
        // На сером кадре без лица — face_detected = false
        EXPECT_FALSE(state.face_detected);
        EXPECT_TRUE(state.alert_distracted);
    } else {
        GTEST_SKIP() << "Model files not found in models/, skipping load test.";
    }
}
