#include "obd_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>

int OBDParser::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return -1; // Возвращаем -1 при ошибке (файл не найден), как указано в требованиях
    }

    records_.clear();
    std::string line;
    int lineNumber = 0;

    // Первая строка CSV – заголовок, пропустить
    if (std::getline(file, line)) {
        lineNumber++;
    }

    // Читаем остальные строки
    int warn_count = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        if (line.empty()) continue;

        OBDRecord record;
        if (parseLineSafe(line, record)) {
            records_.push_back(record);
        } else {
            warn_count++;
            if (warn_count <= 3) {
                std::cerr << "Warning: invalid data format at line " << lineNumber << " - skipping record.\n";
            }
        }
    }
    if (warn_count > 3) {
        std::cerr << "... (" << warn_count << " invalid lines total, suppressed)\n";
    }

    // Метод load() возвращает количество загруженных записей
    return static_cast<int>(records_.size());
}

const OBDRecord& OBDParser::getRecord(int index) const {
    if (index < 0 || index >= static_cast<int>(records_.size())) {
        // бросает std::out_of_range при неверном индексе
        throw std::out_of_range("Index out of range in OBDParser::getRecord");
    }
    return records_[index];
}

const std::vector<OBDRecord>& OBDParser::getRecords() const {
    return records_;
}

BehaviorStyle OBDParser::convertLabelToBehavior(const std::string& label) {
    if (label == "SLOW") return BehaviorStyle::SLOW;
    if (label == "NORMAL") return BehaviorStyle::NORMAL;
    if (label == "AGGRESSIVE") return BehaviorStyle::AGGRESSIVE;
    return BehaviorStyle::UNKNOWN;
}

bool OBDParser::parseLineSafe(const std::string& line, OBDRecord& record) const {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(line);

    // Разбиваем строку (CSV формата) по запятым
    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }
    // Обработка случая, если строка заканчивается на зарятую
    if (!line.empty() && line.back() == ',') {
        tokens.push_back("");
    }

    // Индексы столбцов в нашем CSV:
    // 1: RPM
    // 2: SPEED
    // 3: THROTTLE_POS
    // 7: COOLANT_TEMP
    // 12: FUEL_LEVEL
    // 30: OBD_STATUS (стиль - может отсутствовать)
    if (tokens.size() <= 12) {
        return false;
    }

    try {
        // Конвертация строковых значений в double
        // При пустой строке или некорректном тексте std::stod выбросит исключение
        record.rpm = std::stod(tokens[1]);
        record.speed = std::stod(tokens[2]);
        record.throttle_pos = std::stod(tokens[3]);
        record.coolant_temp = std::stod(tokens[7]);
        record.engine_load = std::stod(tokens[11]);
        record.fuel_level = std::stod(tokens[12]);
        
        // Обработка метки стиля (если есть столбец 30)
        if (tokens.size() > 30 && !tokens[30].empty()) {
            record.style = convertLabelToBehavior(tokens[30]);
        } else {
            record.style = BehaviorStyle::UNKNOWN;
        }

        return true;
    } catch (const std::invalid_argument&) {
        // Пропускаем (возвращаем false), если данные некорректны (например, пустая ячейка для важного поля)
        return false;
    } catch (const std::out_of_range&) {
        // Пропускаем, если число не вмещается в double
        return false;
    }
}
