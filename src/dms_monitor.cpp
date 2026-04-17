#include "dms_monitor.h"
#include <iostream>

DMSMonitor::DMSMonitor() {
    // Загрузка нейросети для обнаружения лица (DNN Caffe)
    try {
        face_net_ = cv::dnn::readNetFromCaffe("models/deploy.prototxt", "models/res10_300x300_ssd_iter_140000.caffemodel");
        if (face_net_.empty()) {
            std::cerr << "DMSMonitor: Face Net пуста. Не удалось загрузить модель." << std::endl;
        }
    } catch (const cv::Exception& e) {
        std::cerr << "DMSMonitor: Ошибка загрузки модели DNN лица: " << e.what() << std::endl;
    }

    // Загрузка Haar каскада для обнаружения глаз
    if (!eye_cascade_.load("models/haarcascade_eye.xml")) {
        std::cerr << "DMSMonitor: Ошибка загрузки haarcascade_eye.xml" << std::endl;
    }
}

DriverState DMSMonitor::analyze(const cv::Mat& frame) {
    DriverState state;
    
    if (frame.empty()) {
        state.face_detected = false;
        state.eyes_open = false;
        state.alert_distracted = true;
        return state;
    }

    // 1. Детекция лица (Оптимизация FPS: запускаем нейросеть только 1 раз в 5 кадров)
    if (frame_counter_ % 5 == 0 || last_face_rect_.area() == 0) {
        last_face_rect_ = detectFace(frame);
    }
    frame_counter_++;

    state.face_rect = last_face_rect_;

    if (state.face_rect.area() > 0) {
        state.face_detected = true;
        
        // 2. Детекция угла поворота головы (аппроксимация по смещению от центра кадра)
        state.head_turn_deg = estimateHeadTurn(frame, state.face_rect);
        state.looking_forward = std::abs(state.head_turn_deg) < 20.0f; // Допуск 20 градусов
        state.alert_distracted = !state.looking_forward;

        // 3. Детекция глаз
        state.eye_openness = estimateEyeOpenness(frame, state.face_rect);
        state.eyes_open = (state.eye_openness > 0.0f);
    } else {
        state.face_detected = false;
        state.eyes_open = false; // Нет лица -> глаза не видны
        state.looking_forward = false;
        state.alert_distracted = true; // Отвлечение внимания, если лица вообще не видно
    }

    // 4. Обновление истории глаз (Буфер увеличен до 30 кадров для 30 FPS)
    eye_history_.push_back(state.eyes_open);
    if (eye_history_.size() > 30) {
        eye_history_.pop_front();
    }

    // 5. Определение усталости (закрыты в 20 из 30 последних кадров)
    int closed_count = 0;
    for (bool open : eye_history_) {
        if (!open) closed_count++;
    }
    
    if (closed_count >= 20 && eye_history_.size() >= 20) {
        state.alert_drowsy = true;
    } else {
        state.alert_drowsy = false;
    }

    return state;
}

cv::Rect DMSMonitor::detectFace(const cv::Mat& frame) {
    if (face_net_.empty()) return cv::Rect(0, 0, 0, 0);

    // Подготовка кадра для нейросети (300x300 - размер, ожидаемый SSD моделью)
    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0, cv::Size(300, 300), cv::Scalar(104.0, 177.0, 123.0), false, false);
    
    face_net_.setInput(blob);
    cv::Mat detection = face_net_.forward();
    
    // Результат имеет размерность [1, 1, N, 7]
    cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

    float max_confidence = 0.0f;
    cv::Rect best_rect(0, 0, 0, 0);

    for (int i = 0; i < detectionMat.rows; i++) {
        float confidence = detectionMat.at<float>(i, 2);
        
        if (confidence > 0.5f && confidence > max_confidence) {
            max_confidence = confidence;
            
            int x1 = static_cast<int>(detectionMat.at<float>(i, 3) * frame.cols);
            int y1 = static_cast<int>(detectionMat.at<float>(i, 4) * frame.rows);
            int x2 = static_cast<int>(detectionMat.at<float>(i, 5) * frame.cols);
            int y2 = static_cast<int>(detectionMat.at<float>(i, 6) * frame.rows);
            
            // Ограничение координат границами кадра
            x1 = std::max(0, x1);
            y1 = std::max(0, y1);
            x2 = std::min(frame.cols - 1, x2);
            y2 = std::min(frame.rows - 1, y2);
            
            best_rect = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
        }
    }
    
    return best_rect;
}

float DMSMonitor::estimateEyeOpenness(const cv::Mat& frame, cv::Rect face_rect) {
    if (eye_cascade_.empty()) return 0.0f;

    // Ищем глаза только в верхней половине лица
    cv::Rect upper_face = face_rect;
    upper_face.height = upper_face.height / 2;
    // Срезаем немного сбоку, чтобы не захватывать уши/волосы (оптимизация)
    upper_face.x += static_cast<int>(upper_face.width * 0.1);
    upper_face.width = static_cast<int>(upper_face.width * 0.8);
    upper_face.y += static_cast<int>(upper_face.height * 0.2);

    // Убедимся, что прямоугольник внутри кадра
    upper_face &= cv::Rect(0, 0, frame.cols, frame.rows);
    if(upper_face.area() == 0) return 0.0f;

    cv::Mat faceROI = frame(upper_face);
    cv::Mat grayROI;
    cv::cvtColor(faceROI, grayROI, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(grayROI, grayROI);

    std::vector<cv::Rect> eyes;
    // Минимальный размер глаз, чтобы отсеять шумы
    int min_eye_size = std::max(10, static_cast<int>(upper_face.width * 0.15));
    eye_cascade_.detectMultiScale(grayROI, eyes, 1.1, 3, 0, cv::Size(min_eye_size, min_eye_size));

    // Возвращаем: есть хотя бы 1 глаз -> 1.0 (закрыты -> 0.0)
    // Каскады Хаара плохо ловят закрытые глаза, поэтому если найдены - открыты.
    if (eyes.size() > 0) return 1.0f;
    return 0.0f;
}

float DMSMonitor::estimateHeadTurn(const cv::Mat& frame, cv::Rect face_rect) {
    // Упрощенная логика: смотрим смещение центра лица от центра кадра.
    // Если камера стоит по центру перед водителем, то сильное смещение влево/вправо 
    // или отсутствие симметрии можно трактовать как отвлечение.
    float face_center_x = face_rect.x + (face_rect.width / 2.0f);
    float frame_center_x = frame.cols / 2.0f;
    
    float offset = (face_center_x - frame_center_x) / frame_center_x; // от -1 до 1
    // Условная конвертация в градусы (до 45 градусов поворота)
    return offset * 45.0f; 
}
