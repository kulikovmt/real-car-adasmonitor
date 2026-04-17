/**
 * @file obd_parser.h
 * @brief OBD-II telemetry CSV parser module.
 */
#pragma once

#include <string>
#include <vector>
#include <stdexcept>

/**
 * @brief Driving behavior style classification.
 */
enum class BehaviorStyle {
    UNKNOWN = -1,
    SLOW = 0,
    NORMAL = 1,
    AGGRESSIVE = 2
};

/**
 * @brief Single OBD-II telemetry record from CSV.
 */
struct OBDRecord {
    double speed;          ///< Vehicle speed (km/h)
    double rpm;            ///< Engine RPM
    double throttle_pos;   ///< Throttle position (%)
    double coolant_temp;   ///< Coolant temperature (C)
    double fuel_level;     ///< Fuel level (%)
    double engine_load;    ///< Engine load (%)
    BehaviorStyle style;   ///< Driving style label (SLOW/NORMAL/AGGRESSIVE)
};

/**
 * @brief Parser for OBD-II telemetry data in CSV format.
 * 
 * Reads CSV files containing vehicle telemetry data,
 * parses each line into OBDRecord structures, and provides
 * access to the parsed records.
 */
class OBDParser {
public:
    OBDParser() = default;

    /**
     * @brief Load and parse a CSV file.
     * @param filepath Path to the CSV file.
     * @return Number of successfully parsed records, or -1 on file error.
     */
    int load(const std::string& filepath);

    /**
     * @brief Get a specific record by index.
     * @param index Record index (0-based).
     * @return Reference to the OBDRecord.
     * @throws std::out_of_range if index is invalid.
     */
    const OBDRecord& getRecord(int index) const;

    /**
     * @brief Get all parsed records.
     * @return Const reference to the vector of OBDRecord.
     */
    const std::vector<OBDRecord>& getRecords() const;

    /**
     * @brief Convert a text label to BehaviorStyle enum.
     * @param label String label ("SLOW", "NORMAL", "AGGRESSIVE").
     * @return Corresponding BehaviorStyle value.
     */
    static BehaviorStyle convertLabelToBehavior(const std::string& label);

private:
    std::vector<OBDRecord> records_;

    /**
     * @brief Safely parse a single CSV line into an OBDRecord.
     * @param line Raw CSV line.
     * @param record Output record.
     * @return true if parsing succeeded, false otherwise.
     */
    bool parseLineSafe(const std::string& line, OBDRecord& record) const;
};
