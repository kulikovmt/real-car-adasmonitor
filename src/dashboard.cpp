#include "dashboard.h"

Dashboard::Dashboard() {
    // Настройки панели
}

void Dashboard::draw(cv::Mat& frame, double speed, double rpm, double coolant, double fuel, double throttle, BehaviorStyle style) {
    // 1. Создаем темный фон панели
    cv::rectangle(frame, cv::Rect(0, 0, panel_width_, frame.rows), cv::Scalar(20, 20, 20), cv::FILLED);

    // Добавляем границу-разделитель (отделяет приборку от камеры)
    cv::line(frame, cv::Point(panel_width_ - 2, 0), cv::Point(panel_width_ - 2, frame.rows), cv::Scalar(100, 100, 100), 2, cv::LINE_AA);

    // 2. Рисуем приборы с Anti-Aliasing и свечением
    bool speed_red = (speed > 90.0);
    drawGauge(frame, cv::Point(90, 80), 55, speed, 140.0, "SPEED", "km/h", speed_red);

    bool rpm_red = (rpm > 4500.0);
    drawGauge(frame, cv::Point(230, 80), 55, rpm, 6000.0, "RPM", "rpm", rpm_red);

    // 3. Рисуем линейные полосы состояния
    drawLinearGauge(frame, cv::Rect(20, 190, 280, 6), coolant, 120.0, "COOLANT TEMP", cv::Scalar(0, 160, 255)); // Оранжевый
    
    cv::Scalar fuel_color = (fuel < 15.0) ? cv::Scalar(20, 20, 255) : cv::Scalar(200, 255, 50); // Зеленоватый
    drawLinearGauge(frame, cv::Rect(20, 250, 280, 6), fuel, 100.0, "FUEL LEVEL", fuel_color);

    drawLinearGauge(frame, cv::Rect(20, 310, 280, 6), throttle, 100.0, "THROTTLE", cv::Scalar(220, 220, 220));

    // 4. Отрисовка стиля вождения
    drawStyleText(frame, cv::Point(20, 380), style);

    // 5. Отрисовка предупреждений
    drawWarnings(frame, cv::Point(20, 440), coolant, fuel);
}

void Dashboard::drawGauge(cv::Mat& frame, cv::Point center, int radius, double value, double max_value, 
                          const std::string& label, const std::string& unit, bool is_red_zone) {
    
    // Фон прибора - темное кольцо
    cv::circle(frame, center, radius, cv::Scalar(50, 50, 50), 4, cv::LINE_AA);

    if (value > max_value) value = max_value;
    if (value < 0) value = 0;

    double angle_span = 270.0;
    double start_angle = 135.0; 
    double fill_angle = (value / max_value) * angle_span;

    // В OpenCV цвета задаются как BGR (Blue, Green, Red)
    cv::Scalar core_color = is_red_zone ? cv::Scalar(50, 50, 255) : cv::Scalar(255, 230, 50); // Красный / Голубой (Cyan)
    cv::Scalar glow_color = is_red_zone ? cv::Scalar(0, 0, 150)   : cv::Scalar(100, 100, 0);

    if (fill_angle > 0) {
        // Свечение (Glow effect) - толстая линия темного тона
        cv::ellipse(frame, center, cv::Size(radius, radius), 0, start_angle, start_angle + fill_angle, glow_color, 12, cv::LINE_AA);
        // Ядро (Core) - тонкая яркая линия
        cv::ellipse(frame, center, cv::Size(radius, radius), 0, start_angle, start_angle + fill_angle, core_color, 3, cv::LINE_AA);
    }

    // Значение по центру
    std::string val_str = std::to_string(static_cast<int>(value));
    int baseline = 0;
    cv::Size text_size = cv::getTextSize(val_str, cv::FONT_HERSHEY_DUPLEX, 0.8, 2, &baseline);
    cv::Point text_pos(center.x - text_size.width / 2, center.y + 8);
    
    // Свечение текста
    cv::putText(frame, val_str, text_pos, cv::FONT_HERSHEY_DUPLEX, 0.8, glow_color, 5, cv::LINE_AA);
    cv::putText(frame, val_str, text_pos, cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);

    // Название
    cv::putText(frame, label, cv::Point(center.x - 24, center.y + radius + 18), 
                cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
    cv::putText(frame, unit, cv::Point(center.x - 16, center.y + radius + 32), 
                cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(130, 130, 130), 1, cv::LINE_AA);
}

void Dashboard::drawLinearGauge(cv::Mat& frame, cv::Rect rect, double value, double max_value, 
                                const std::string& label, const cv::Scalar& color) {
                                    
    // Фон полоски (стильная темная линия со сглаживанием)
    cv::line(frame, cv::Point(rect.x, rect.y), cv::Point(rect.x + rect.width, rect.y), cv::Scalar(50, 50, 50), rect.height, cv::LINE_AA);

    if (value > max_value) value = max_value;
    if (value < 0) value = 0;
    
    int fill_width = static_cast<int>((value / max_value) * rect.width);
    if (fill_width > 0) {
        // Уменьшенный вариант цвета для свечения (Opacity)
        cv::Scalar glow_col(color[0]*0.5, color[1]*0.5, color[2]*0.5);
        
        // Эффект свечения
        cv::line(frame, cv::Point(rect.x, rect.y), cv::Point(rect.x + fill_width, rect.y), glow_col, rect.height + 6, cv::LINE_AA);
        // Ядро
        cv::line(frame, cv::Point(rect.x, rect.y), cv::Point(rect.x + fill_width, rect.y), color, rect.height, cv::LINE_AA);
    }

    // Текст названия
    cv::putText(frame, label, cv::Point(rect.x, rect.y - 12), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
    
    // Текст значения справа над полосой
    std::string val_str = std::to_string(static_cast<int>(value));
    cv::putText(frame, val_str, cv::Point(rect.x + rect.width - 35, rect.y - 10), 
                cv::FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
}

void Dashboard::drawStyleText(cv::Mat& frame, cv::Point pos, BehaviorStyle style) {
    std::string text = "STYLE: ";
    cv::Scalar color;

    switch(style) {
        case BehaviorStyle::SLOW:
            text += "SMOOTH";
            color = cv::Scalar(255, 200, 50); // Cyan glow
            break;
        case BehaviorStyle::NORMAL:
            text += "BALANCED";
            color = cv::Scalar(100, 255, 100); // Green glow
            break;
        case BehaviorStyle::AGGRESSIVE:
            text += "AGGRESSIVE";
            color = cv::Scalar(50, 50, 255); // Red glow
            break;
        default:
            text += "ANALYZING...";
            color = cv::Scalar(150, 150, 150);
            break;
    }

    cv::putText(frame, text, pos, cv::FONT_HERSHEY_DUPLEX, 0.65, color, 1, cv::LINE_AA);
}

void Dashboard::drawWarnings(cv::Mat& frame, cv::Point pos, double coolant, double fuel) {
    int y_offset = 0;
    cv::Scalar warn_color(0, 0, 255); // Red
    if (coolant > 100.0) {
        cv::putText(frame, "[!] OVERHEAT WARNING", cv::Point(pos.x, pos.y + y_offset), 
                    cv::FONT_HERSHEY_DUPLEX, 0.55, warn_color, 1, cv::LINE_AA);
        y_offset += 25;
    }
    if (fuel < 15.0) {
        cv::putText(frame, "[!] LOW FUEL WARNING", cv::Point(pos.x, pos.y + y_offset), 
                    cv::FONT_HERSHEY_DUPLEX, 0.55, warn_color, 1, cv::LINE_AA);
    }
}
