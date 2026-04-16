#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include "obd_parser.h"
#include "onnx_classifier.h"

// Вспомогательная функция для перевода числовой метки в читаемый текст
std::string labelToString(int label) {
    if (label == 0) return "SLOW";
    if (label == 1) return "NORMAL";
    if (label == 2) return "AGGRESSIVE";
    return "UNKNOWN";
}

int main() {
    std::cout << "=== Загрузка данных и инициализация модели ===\n";

    OBDParser parser;
    if (parser.load("../data/dataset.csv") <= 0) {
        std::cerr << "Ошибка: Не удалось загрузить данные телеметрии." << std::endl;
        return 1;
    }

    ONNXClassifier classifier;
    if (!classifier.initialize("../models/driver_classifier.onnx", "../models/normalization_params.json")) {
        std::cerr << "Ошибка: Не удалось инициализировать классификатор ONNX." << std::endl;
        return 1;
    }

    std::cout << "\n=== Тестирование модели (первые 20 записей) ===\n";
    
    // Форматирование "шапки" таблицы
    std::cout << std::left 
              << std::setw(5)  << "N"
              << std::setw(15) << "Истинная метка" 
              << std::setw(15) << "Предсказание" 
              << std::setw(15) << "Уверенность" 
              << "Совпадение"  << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    int correct_predictions = 0;
    // Защита: проверяем сколько записей загружено (минимум между 20 и фактом)
    int items_to_test = std::min(20, static_cast<int>(parser.getRecords().size()));

    for (int i = 0; i < items_to_test; ++i) {
        const auto& record = parser.getRecord(i);

        // Собираем необходимые 6 фичей в строгий массив для классификатора
        std::array<float, 6> raw_features = {
            static_cast<float>(record.speed),
            static_cast<float>(record.rpm),
            static_cast<float>(record.throttle_pos),
            static_cast<float>(record.coolant_temp),
            static_cast<float>(record.fuel_level),
            static_cast<float>(record.engine_load)
        };

        // Запускаем предсказание
        ClassificationResult result = classifier.predict(raw_features);

        // Истинная метка выгружается из парсера OBD
        int true_label = static_cast<int>(record.style);
        bool is_correct = (true_label == result.label);
        
        if (is_correct) {
            correct_predictions++;
        }

        // Печатаем строку таблицы
        std::cout << std::left 
                  << std::setw(5)  << (i + 1)
                  << std::setw(15) << labelToString(true_label)
                  << std::setw(15) << labelToString(result.label)
                  << std::fixed << std::setprecision(2) << (result.confidence * 100.0f) << "%"
                  << std::string(8, ' ') // отступ для ровности колонки
                  << (is_correct ? " OK" : " ERR") << std::endl;
    }

    // Подсчет точности (Accuracy)
    float accuracy = (static_cast<float>(correct_predictions) / items_to_test) * 100.0f;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "Точность (Accuracy) на " << items_to_test << " записях: " 
              << std::fixed << std::setprecision(1) << accuracy << "%\n";

    return 0;
}
