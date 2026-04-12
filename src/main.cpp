#include <iostream>
#include <iomanip>
#include "obd_parser.h"

const char* VERSION = "1.0.0";

int main() {
    std::cout << "RealCarMonitor: Welcome!" << std::endl;
    std::cout << "Version: " << VERSION << std::endl;

    OBDParser parser;
    // Поскольку сборка идет в папке build, обращаемся к директории data через "../" 
    std::string datasetPath = "../data/dataset.csv";
    
    std::cout << "\nLoading dataset from " << datasetPath << "..." << std::endl;
    int loadedCount = parser.load(datasetPath);
    
    if (loadedCount < 0) {
        std::cerr << "Failed to load dataset! Please check if the file " << datasetPath << " exists." << std::endl;
        return 1;
    }

    std::cout << "Successfully loaded " << loadedCount << " records.\n";

    // Вывод первых 5 записей
    std::cout << "\n--- First 5 Records ---\n";
    for (int i = 0; i < 5 && i < loadedCount; ++i) {
        const auto& record = parser.getRecord(i);
        std::cout << "Record " << i + 1 << ": "
                  << "RPM=" << std::fixed << std::setprecision(2) << record.rpm << ", "
                  << "SPEED=" << record.speed << ", "
                  << "THROTTLE=" << record.throttle_pos << ", "
                  << "COOLANT=" << record.coolant_temp << ", "
                  << "FUEL=" << record.fuel_level << ", "
                  << "STYLE_ID=" << static_cast<int>(record.style) << "\n";
    }

    // Считаем статистику по классам
    int slowCount = 0;
    int normalCount = 0;
    int aggressiveCount = 0;

    for (const auto& record : parser.getRecords()) {
        if (record.style == BehaviorStyle::SLOW) slowCount++;
        else if (record.style == BehaviorStyle::NORMAL) normalCount++;
        else if (record.style == BehaviorStyle::AGGRESSIVE) aggressiveCount++;
    }

    // Вывод статистики
    std::cout << "\n--- Behavior Statistics ---\n";
    std::cout << "SLOW:       " << slowCount << "\n";
    std::cout << "NORMAL:     " << normalCount << "\n";
    std::cout << "AGGRESSIVE: " << aggressiveCount << "\n";

    return 0;
}
