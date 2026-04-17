#include "dms_hud.h"

DMSHUD::DMSHUD() {}

void DMSHUD::drawCornerRect(cv::Mat& frame, cv::Rect rect, const cv::Scalar& color, int thickness, int length) {
    // Рисует только уголки прямоугольника для создания HUD-рамки детектирования
    int x = rect.x, y = rect.y, w = rect.width, h = rect.height;
    
    // Top-Left corner
    cv::line(frame, cv::Point(x, y), cv::Point(x + length, y), color, thickness, cv::LINE_AA);
    cv::line(frame, cv::Point(x, y), cv::Point(x, y + length), color, thickness, cv::LINE_AA);
    
    // Top-Right corner
    cv::line(frame, cv::Point(x + w, y), cv::Point(x + w - length, y), color, thickness, cv::LINE_AA);
    cv::line(frame, cv::Point(x + w, y), cv::Point(x + w, y + length), color, thickness, cv::LINE_AA);
    
    // Bottom-Left corner
    cv::line(frame, cv::Point(x, y + h), cv::Point(x + length, y + h), color, thickness, cv::LINE_AA);
    cv::line(frame, cv::Point(x, y + h), cv::Point(x, y + h - length), color, thickness, cv::LINE_AA);
    
    // Bottom-Right corner
    cv::line(frame, cv::Point(x + w, y + h), cv::Point(x + w - length, y + h), color, thickness, cv::LINE_AA);
    cv::line(frame, cv::Point(x + w, y + h), cv::Point(x + w, y + h - length), color, thickness, cv::LINE_AA);
}

void DMSHUD::render(cv::Mat& frame, const DriverState& state) {
    // Базовый цвет рамки (Зеленый = ОК, Оранжевый = Внимание)
    cv::Scalar frame_color = cv::Scalar(0, 255, 0); // BGR
    if (state.alert_drowsy || state.alert_distracted) {
        frame_color = cv::Scalar(0, 100, 255); // Оранжевый
    }
    
    // 1. Отрисовка рамки вокруг лица
    if (state.face_detected) {
        int corner_length = std::max(10, state.face_rect.width / 5);
        drawCornerRect(frame, state.face_rect, frame_color, 3, corner_length);
        
        // Подпись "DRIVER" над рамкой
        cv::putText(frame, "DRIVER", cv::Point(state.face_rect.x, std::max(15, state.face_rect.y - 10)), 
                    cv::FONT_HERSHEY_DUPLEX, 0.5, frame_color, 1, cv::LINE_AA);
    }

    // 2. Информационная панель справа вверху
    int hud_w = 200;
    int hud_h = 100;
    int hud_x = frame.cols - hud_w - 20;
    int hud_y = 30;
    
    // Создаем полупрозрачную подложку под статусы
    cv::Mat overlay;
    frame.copyTo(overlay);
    cv::rectangle(overlay, cv::Rect(hud_x - 10, hud_y - 20, hud_w, hud_h), cv::Scalar(20, 20, 20), cv::FILLED);
    cv::addWeighted(overlay, 0.7, frame, 0.3, 0, frame);

    // Заголовок панели
    cv::putText(frame, "DMS STATUS", cv::Point(hud_x, hud_y), cv::FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    
    // Индикатор Глаз
    cv::Scalar eye_color = state.eyes_open ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255); // Зеленый / Красный
    std::string eye_text = state.eyes_open ? "EYES: OPEN" : "EYES: CLOSED";
    cv::circle(frame, cv::Point(hud_x + 8, hud_y + 25), 6, eye_color, cv::FILLED, cv::LINE_AA);
    cv::putText(frame, eye_text, cv::Point(hud_x + 25, hud_y + 30), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);

    // Индикатор Положения Головы
    cv::Scalar head_color = state.looking_forward ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
    std::string head_text = state.looking_forward ? "HEAD: FRONT" : "HEAD: TURNED";
    cv::circle(frame, cv::Point(hud_x + 8, hud_y + 50), 6, head_color, cv::FILLED, cv::LINE_AA);
    cv::putText(frame, head_text, cv::Point(hud_x + 25, hud_y + 55), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);

    // 3. Вывод критических алертов
    if (state.alert_drowsy) {
        std::string alert = "DROWSINESS ALERT";
        int baseline = 0;
        cv::Size text_size = cv::getTextSize(alert, cv::FONT_HERSHEY_DUPLEX, 1.2, 3, &baseline);
        cv::Point pos((frame.cols - text_size.width) / 2, frame.rows / 2);
        
        // Тень/Свечение текста
        cv::putText(frame, alert, pos, cv::FONT_HERSHEY_DUPLEX, 1.2, cv::Scalar(0, 0, 200), 8, cv::LINE_AA); 
        // Основной текст
        cv::putText(frame, alert, pos, cv::FONT_HERSHEY_DUPLEX, 1.2, cv::Scalar(0, 100, 255), 2, cv::LINE_AA); 
    }

    if (state.alert_distracted) {
        // Красная предупреждающая полоса внизу кадра
        int border_thickness = 15;
        cv::rectangle(frame, cv::Rect(0, frame.rows - border_thickness, frame.cols, border_thickness), cv::Scalar(0, 0, 255), cv::FILLED);
        
        std::string alert = "DISTRACTION WARNING!";
        int baseline = 0;
        cv::Size text_size = cv::getTextSize(alert, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseline);
        cv::Point pos((frame.cols - text_size.width) / 2, frame.rows - border_thickness - 10);
        
        cv::putText(frame, alert, pos, cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
    }
}
