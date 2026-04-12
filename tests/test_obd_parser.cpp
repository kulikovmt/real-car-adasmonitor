#include <gtest/gtest.h>
#include "obd_parser.h"
#include <fstream>
#include <cstdio>
#include <string>

// 1. Тест: конвертация меток (SLOW -> 0, NORMAL -> 1, AGGRESSIVE -> 2)
TEST(OBDParserTest, ConvertLabel) {
    EXPECT_EQ(OBDParser::convertLabelToBehavior("SLOW"), BehaviorStyle::SLOW);
    EXPECT_EQ(OBDParser::convertLabelToBehavior("NORMAL"), BehaviorStyle::NORMAL);
    EXPECT_EQ(OBDParser::convertLabelToBehavior("AGGRESSIVE"), BehaviorStyle::AGGRESSIVE);
    
    // Проверяем само численное представление
    EXPECT_EQ(static_cast<int>(BehaviorStyle::SLOW), 0);
    EXPECT_EQ(static_cast<int>(BehaviorStyle::NORMAL), 1);
    EXPECT_EQ(static_cast<int>(BehaviorStyle::AGGRESSIVE), 2);
}

// 2. Тест: загрузка несуществующего файла возвращает -1
TEST(OBDParserTest, LoadMissingFile) {
    OBDParser parser;
    int result = parser.load("file_does_not_exist.csv");
    EXPECT_EQ(result, -1);
}

// 3. Тест: getRecord() бросает исключение при неверном индексе
TEST(OBDParserTest, GetRecordThrowsOutOfRange) {
    OBDParser parser; // Парсер пустой
    EXPECT_THROW(parser.getRecord(0), std::out_of_range);
    EXPECT_THROW(parser.getRecord(-1), std::out_of_range);
    EXPECT_THROW(parser.getRecord(100), std::out_of_range);
}

// 4. Тест: парсинг корректного CSV файла
TEST(OBDParserTest, ParseValidCSV) {
    std::string testFile = "test_valid.csv";
    std::ofstream out(testFile);
    // Заголовок (31 колонка)
    out << "T,RPM,SPEED,TPOS,4,5,6,CTEMP,8,9,10,11,FLEVEL,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,OBDS\n";
    // 1 корректная строка
    out << "Time,685.0,20.5,21.17,4,5,6,90.5,8,9,10,11,54.1,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,SLOW\n";
    out.close();

    OBDParser parser;
    int result = parser.load(testFile);
    EXPECT_EQ(result, 1);

    const OBDRecord& record = parser.getRecord(0);
    EXPECT_DOUBLE_EQ(record.rpm, 685.0);
    EXPECT_DOUBLE_EQ(record.speed, 20.5);
    EXPECT_DOUBLE_EQ(record.throttle_pos, 21.17);
    EXPECT_DOUBLE_EQ(record.coolant_temp, 90.5);
    EXPECT_DOUBLE_EQ(record.fuel_level, 54.1);
    EXPECT_EQ(record.style, BehaviorStyle::SLOW);

    std::remove(testFile.c_str());
}

// 5. Тест: парсинг файла с некорректной строкой
TEST(OBDParserTest, ParseWithInvalidLine) {
    std::string testFile = "test_invalid.csv";
    std::ofstream out(testFile);
    out << "T,RPM,SPEED,TPOS,4,5,6,CTEMP,8,9,10,11,FLEVEL,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,OBDS\n";
    // 1: Нормальная
    out << "T1,685.0,20.5,21.17,4,5,6,90.5,8,9,10,11,54.1,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,SLOW\n";
    // 2: Некорректная SPEED (текст ERROR)
    out << "T2,700.0,ERROR,21.17,4,5,6,90.5,8,9,10,11,54.1,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,NORMAL\n";
    // 3: Нормальная
    out << "T3,710.0,25.0,22.0,4,5,6,91.0,8,9,10,11,54.1,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,FAST\n";
    out.close();

    OBDParser parser;
    int result = parser.load(testFile);
    
    // Должна загрузиться 1 и 3 строка, 2 пропустится (и выведет предупреждение в std::cerr)
    EXPECT_EQ(result, 2);

    const auto& records = parser.getRecords();
    EXPECT_EQ(records.size(), 2);
    
    // Проверяем, что загрузились нужные строки
    EXPECT_DOUBLE_EQ(records[0].rpm, 685.0);
    EXPECT_DOUBLE_EQ(records[1].rpm, 710.0);

    // Удаляем временный файл
    std::remove(testFile.c_str());
}
