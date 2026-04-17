#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include "dms_monitor.h"
#include "dms_hud.h"
#include "dashboard.h"
#include "obd_parser.h"
#include "onnx_classifier.h"

// Шаг 7.2: Создание SharedState
struct SharedState {
    std::atomic<bool> running{true};
    std::mutex mtx;
    
    OBDRecord current_obd = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, BehaviorStyle::UNKNOWN};
    ClassificationResult obd_class = {1, 0.0f, {0,0,0}};
    
    int total_obd_records = 0;
    int drowsiness_alerts = 0;
    int distraction_alerts = 0;
    int aggressive_alerts = 0;
};

// Функция для получения текущего времени (для логов)
std::string getCurrentTimeStr() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Шаг 7.3: Поток OBD (10 Hz)
void obd_thread_func(SharedState* state) {
    OBDParser parser;
    // Грузим тестовый или настоящий датасет
    if (parser.load("data/dataset.csv") <= 0) {
        std::cerr << "[OBD Thread] ВНИМАНИЕ: Нет data/dataset.csv. Использую заглушки.\n";
    }

    ONNXClassifier classifier;
    if (!classifier.initialize("models/driver_classifier.onnx", "models/normalization_params.json")) {
        std::cerr << "[OBD Thread] ВНИМАНИЕ: Не удалось инициализировать ONNX. Классификация не работает.\n";
    }

    const auto& records = parser.getRecords();
    size_t index = 0;

    while (state->running) {
        OBDRecord rec = {0, 0, 0, 0, 0, 0, BehaviorStyle::NORMAL};
        ClassificationResult res = {1, 1.0f, {0,1,0}};
        
        if (!records.empty()) {
            if (index >= records.size()) index = 0; // Зацикливаем
            rec = records[index++];
            
            std::array<float, 6> features = {
                static_cast<float>(rec.speed),
                static_cast<float>(rec.rpm),
                static_cast<float>(rec.throttle_pos),
                static_cast<float>(rec.coolant_temp),
                static_cast<float>(rec.fuel_level),
                static_cast<float>(rec.engine_load)
            };
            res = classifier.predict(features);
        }

        // Обновление SharedState
        {
            std::lock_guard<std::mutex> lock(state->mtx);
            state->current_obd = rec;
            state->obd_class = res;
            state->total_obd_records++;
            
            if (res.label == 2) { // 2 = AGGRESSIVE
                state->aggressive_alerts++;
            }
        }

        // 10 Hz = 100 мс
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Шаг 7.4: Главный цикл
int main() {
    std::cout << "Starting Final ADAS Multi-thread System...\n";

    SharedState state;
    std::thread obd_thread(obd_thread_func, &state);

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Не удалось открыть веб-камеру.\n";
        state.running = false;
        obd_thread.join();
        return -1;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    DMSMonitor dms_monitor;
    DMSHUD dms_hud;
    Dashboard dashboard;

    cv::VideoWriter writer("output/result_situation2.mp4", 
                           cv::VideoWriter::fourcc('M', 'P', '4', 'V'), 
                           30.0, cv::Size(1280, 480));

    std::ofstream alert_log("output/dms_alerts.log", std::ios::app);
    alert_log << "=== New Session Started: " << getCurrentTimeStr() << " ===\n";

    cv::Mat frame;
    cv::Mat canvas(480, 1280, CV_8UC3, cv::Scalar(15, 15, 15));
    
    bool was_drowsy = false;
    bool was_distracted = false;
    bool is_paused = false;
    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        if (!is_paused) {
            cap >> frame;
            if (frame.empty()) break;
            
            cv::flip(frame, frame, 1);
            
            // Анализ водителя
            DriverState dms_state = dms_monitor.analyze(frame);

            // Чтение данных потока OBD
            OBDRecord obd_data;
            ClassificationResult ai_res;
            
            {
                std::lock_guard<std::mutex> lock(state.mtx);
                obd_data = state.current_obd;
                ai_res = state.obd_class;
                
                // Фиксация новых DMS-алертов для логов
                if (dms_state.alert_drowsy && !was_drowsy) {
                    state.drowsiness_alerts++;
                    alert_log << "[" << getCurrentTimeStr() << "] ALERT: Drowsiness detected!\n";
                    was_drowsy = true;
                } else if (!dms_state.alert_drowsy) {
                    was_drowsy = false;
                }

                if (dms_state.alert_distracted && !was_distracted) {
                    state.distraction_alerts++;
                    alert_log << "[" << getCurrentTimeStr() << "] WARNING: Distraction detected!\n";
                    was_distracted = true;
                } else if (!dms_state.alert_distracted) {
                    was_distracted = false;
                }
            }

            // Рендер HUD на кадре с камеры
            dms_hud.render(frame, dms_state);
            
            // Вставка камеры в правую половину 1280x480 (координаты X: 640..1280)
            frame.copyTo(canvas(cv::Rect(640, 0, 640, 480)));

            // Подготовка левой половины
            cv::Mat dashboard_area = canvas(cv::Rect(0, 0, 640, 480));
            dashboard_area.setTo(cv::Scalar(15, 15, 15));
            
            // Центрирование приборной панели (320px) внутри левых (640px)
            // Отступ слева = (640 - 320) / 2 = 160
            cv::Mat dashboard_roi = canvas(cv::Rect(160, 0, 320, 480));
            BehaviorStyle style = static_cast<BehaviorStyle>(ai_res.label);
            dashboard.draw(dashboard_roi, obd_data.speed, obd_data.rpm, obd_data.coolant_temp, obd_data.fuel_level, obd_data.throttle_pos, style);

            // Отрисовка текста паузы
            cv::imshow("ADAS Monitor - Multi Thread Final", canvas);
            if (writer.isOpened()) {
                writer.write(canvas);
            }
        } else {
            // Если на паузе, показываем текст
            cv::Mat paused_canvas = canvas.clone();
            cv::putText(paused_canvas, "PAUSED (Press SPACE to resume)", cv::Point(400, 240), 
                        cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar(0, 200, 255), 2, cv::LINE_AA);
            cv::imshow("ADAS Monitor - Multi Thread Final", paused_canvas);
        }

        // Обработка клавиш управления (Q - выход, SPACE - пауза, S - скриншот)
        char key = (char)cv::waitKey(10);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        } else if (key == ' ' || key == 'p') {
            is_paused = !is_paused;
        } else if (key == 's' || key == 'S') {
            cv::imwrite("output/screenshot.jpg", canvas);
            std::cout << "[INFO] Скриншот сохранен в output/screenshot.jpg\n";
        }
        
        if (cv::getWindowProperty("ADAS Monitor - Multi Thread Final", cv::WND_PROP_VISIBLE) < 1) {
            break;
        }
    }

    // Завершение работы
    state.running = false;
    obd_thread.join();
    writer.release();
    alert_log.close();

    // Шаг 7.5: Вывод статистики
    auto end_time = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "\n=== СТАТИСТИКА РАБОТЫ (Финальная система) ===\n";
    std::cout << "Время работы системы: " << std::fixed << std::setprecision(2) << duration << " секунд\n";
    std::cout << "Обработано записей телеметрии (OBD): " << state.total_obd_records << "\n";
    std::cout << "Алерты Усталости (DMS): " << state.drowsiness_alerts << "\n";
    std::cout << "Алерты Отвлечения (DMS): " << state.distraction_alerts << "\n";
    std::cout << "Алерты Агрессивного Вождения (ONNX): " << state.aggressive_alerts << "\n";
    std::cout << "Всего алертов за поездку: " << (state.drowsiness_alerts + state.distraction_alerts + state.aggressive_alerts) << "\n";
    std::cout << "===============================================\n\n";

    return 0;
}
