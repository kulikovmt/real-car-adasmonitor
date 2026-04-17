#pragma once

#include <string>
#include <vector>
#include <stdexcept>

// Перечисление для стилей вождения (для удобства конвертации "SLOW"/"NORMAL"/"AGGRESSIVE")
enum class BehaviorStyle {
    UNKNOWN = -1,
    SLOW = 0,
    NORMAL = 1,
    AGGRESSIVE = 2
};

// Структура хранения данных одной считанной строки OBD-II
struct OBDRecord {
    double speed;          // Скорость (SPEED)
    double rpm;            // Обороты двигателя (RPM)
    double throttle_pos;   // Положение дроссельной заслонки (THROTTLE_POS)
    double coolant_temp;   // Температура охлаждающей жидкости (COOLANT_TEMP)
    double fuel_level;     // Уровень топлива (FUEL_LEVEL)
    double engine_load;    // Нагрузка на двигатель (ENGINE_LOAD)
    BehaviorStyle style;   // Метка стиля вождения (SLOW/NORMAL/AGGRESSIVE)
};

// Парсер данных OBD-II телеметрии (CSV)
class OBDParser {
public:
    OBDParser() = default;

    // Возвращает количество загруженных записей или -1 при ошибке (например, файл не найден)
    int load(const std::string& filepath);

    // Возвращает конкретную запись. Бросает std::out_of_range при неверном индексе
    const OBDRecord& getRecord(int index) const;

    // Получить все распарсенные записи
    const std::vector<OBDRecord>& getRecords() const;

    // Метод для конвертации текстовой строки стиля вождения в enum/число (SLOW -> 0, и т.д.)
    static BehaviorStyle convertLabelToBehavior(const std::string& label);

private:
    std::vector<OBDRecord> records_;

    // Метод для безопасного парсинга одной строки с обработкой неверного формата данных
    bool parseLineSafe(const std::string& line, OBDRecord& record) const;
};
