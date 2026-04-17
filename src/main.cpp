#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
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

// Шаг 7.3: Поток OBD (10 Hz) — потоковое чтение CSV
void obd_thread_func(SharedState* state) {
    try {
        std::cout << "[OBD Thread] Запуск потока телеметрии...\n";
        
        std::ifstream csv_file("data/dataset.csv");
        if (!csv_file.is_open()) {
            std::cerr << "[OBD Thread] ОШИБКА: не удалось открыть data/dataset.csv\n";
            return;
        }
        
        // Пропускаем заголовок
        std::string header_line;
        std::getline(csv_file, header_line);
        std::cout << "[OBD Thread] CSV открыт. Начинаю потоковое чтение...\n";

        OBDParser parser;  // Используем только для parseLineSafe
        std::string line;
        int line_num = 1;
        int ok_count = 0;

        while (state->running) {
            // Читаем следующую строку из CSV
            if (!std::getline(csv_file, line)) {
                // Конец файла — возвращаемся к началу
                csv_file.clear();
                csv_file.seekg(0);
                std::getline(csv_file, header_line); // Пропустить заголовок
                line_num = 1;
                continue;
            }
            line_num++;
            
            if (line.empty()) continue;
            
            // Парсим строку
            OBDRecord rec;
            // Вручную парсим строку (ищем нужные столбцы)
            std::vector<std::string> tokens;
            std::string token;
            std::stringstream ss(line);
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() <= 12) {
                continue; // Строка слишком короткая
            }
            
            try {
                // Индексы из CSV: 1=RPM, 2=SPEED, 3=THROTTLE_POS, 7=COOLANT_TEMP, 11=ENGINE_LOAD, 12=FUEL_LEVEL
                rec.speed = tokens[2].empty() ? 0.0 : std::stod(tokens[2]);
                rec.rpm = tokens[1].empty() ? 0.0 : std::stod(tokens[1]);
                rec.throttle_pos = tokens[3].empty() ? 0.0 : std::stod(tokens[3]);
                rec.coolant_temp = tokens.size() > 7 && !tokens[7].empty() ? std::stod(tokens[7]) : 85.0;
                rec.engine_load = tokens.size() > 11 && !tokens[11].empty() ? std::stod(tokens[11]) : 30.0;
                rec.fuel_level = tokens.size() > 12 && !tokens[12].empty() ? std::stod(tokens[12]) : 50.0;
                
                // Стиль вождения (столбец 30)
                if (tokens.size() > 30 && !tokens[30].empty()) {
                    rec.style = OBDParser::convertLabelToBehavior(tokens[30]);
                } else {
                    rec.style = BehaviorStyle::NORMAL;
                }
            } catch (...) {
                continue; // Пропускаем битую строку
            }

            // Результат классификации из CSV-метки
            ClassificationResult res;
            res.label = static_cast<int>(rec.style);
            if (res.label < 0 || res.label > 2) res.label = 1;
            res.confidence = 1.0f;
            res.scores = {0, 0, 0};
            res.scores[res.label] = 1.0f;

            // Обновление SharedState
            {
                std::lock_guard<std::mutex> lock(state->mtx);
                state->current_obd = rec;
                state->obd_class = res;
                state->total_obd_records++;
                
                if (res.label == 2) {
                    state->aggressive_alerts++;
                }
            }
            
            ok_count++;
            if (ok_count == 1) {
                std::cout << "[OBD Thread] Первая запись отправлена: speed=" << rec.speed << " rpm=" << rec.rpm << "\n";
            }

            // 10 Hz = 100 мс
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "[OBD Thread] Поток завершён. Обработано: " << ok_count << " записей.\n";
    } catch (const std::exception& e) {
        std::cerr << "[OBD Thread] КРИТИЧЕСКАЯ ОШИБКА: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "[OBD Thread] НЕИЗВЕСТНАЯ ОШИБКА в потоке OBD.\n";
    }
}

// Шаг 7.4: Главный цикл
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
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

            // DEBUG: Записываем значения OBD в файл (каждые 30 кадров)
            static int dbg_counter = 0;
            static std::ofstream dbg_file("output/debug_obd.txt");
            if (dbg_counter++ % 30 == 0) {
                dbg_file << "[DEBUG] OBD: speed=" << obd_data.speed 
                         << " rpm=" << obd_data.rpm
                         << " coolant=" << obd_data.coolant_temp
                         << " fuel=" << obd_data.fuel_level
                         << " throttle=" << obd_data.throttle_pos
                         << " total_records=" << state.total_obd_records << "\n";
                dbg_file.flush();
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

    std::cout << "\n=== SESSION STATISTICS (Final System) ===\n";
    std::cout << "Session duration: " << std::fixed << std::setprecision(2) << duration << " sec\n";
    std::cout << "OBD records processed: " << state.total_obd_records << "\n";
    std::cout << "Drowsiness alerts (DMS): " << state.drowsiness_alerts << "\n";
    std::cout << "Distraction alerts (DMS): " << state.distraction_alerts << "\n";
    std::cout << "Aggressive driving alerts: " << state.aggressive_alerts << "\n";
    std::cout << "Total alerts: " << (state.drowsiness_alerts + state.distraction_alerts + state.aggressive_alerts) << "\n";
    std::cout << "=========================================\n\n";

    return 0;
}
