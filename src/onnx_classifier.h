#pragma once

#ifdef _WIN32
#include <windows.h>
// Fix GCC 15 syntax error with windows.h _stdcall macro expansion
#undef _stdcall
#define _stdcall __stdcall

// Include the real one so we can forcefully override its macros.
#include <specstrings.h>

#undef _In_
#define _In_
#undef _In_z_
#define _In_z_
#undef _In_opt_
#define _In_opt_
#undef _In_opt_z_
#define _In_opt_z_
#undef _Out_
#define _Out_
#undef _Outptr_
#define _Outptr_
#undef _Out_opt_
#define _Out_opt_
#undef _Inout_
#define _Inout_
#undef _Inout_opt_
#define _Inout_opt_
#undef _Frees_ptr_opt_
#define _Frees_ptr_opt_
#undef _Ret_maybenull_
#define _Ret_maybenull_
#undef _Ret_notnull_
#define _Ret_notnull_
#undef _Check_return_
#define _Check_return_
#undef _Outptr_result_maybenull_
#define _Outptr_result_maybenull_
#undef _In_reads_
#define _In_reads_(X)
#undef _Inout_updates_
#define _Inout_updates_(X)
#undef _Out_writes_
#define _Out_writes_(X)
#undef _Inout_updates_all_
#define _Inout_updates_all_(X)
#undef _Out_writes_bytes_all_
#define _Out_writes_bytes_all_(X)
#undef _Out_writes_all_
#define _Out_writes_all_(X)
#undef _Success_
#define _Success_(X)
#undef _Outptr_result_buffer_maybenull_
#define _Outptr_result_buffer_maybenull_(X)
#undef _Return_type_success_
#define _Return_type_success_(X)
#undef _Outptr_result_buffer_
#define _Outptr_result_buffer_(X)
#undef _Out_writes_to_
#define _Out_writes_to_(X, Y)

#endif

#include <string>
#include <vector>
#include <array>
#define ORT_API_VERSION 17
#include <onnxruntime_cxx_api.h>
#include "obd_parser.h"

struct ClassificationResult {
    int label;                   // 0 (SLOW), 1 (NORMAL), 2 (AGGRESSIVE)
    float confidence;            // Уверенность сети от 0.0 до 1.0
    std::array<float, 3> scores; // Вероятности для всех 3 классов
};

class ONNXClassifier {
public:
    ONNXClassifier();
    ~ONNXClassifier() = default;

    bool initialize(const std::string& modelPath, const std::string& jsonPath);
    ClassificationResult predict(const std::array<float, 6>& raw_features);

private:
    std::vector<double> mean_;
    std::vector<double> std_;

    Ort::Env env_{nullptr};
    Ort::Session session_{nullptr};
    Ort::AllocatorWithDefaultOptions allocator_;

    std::vector<std::string> input_node_names_;
    std::vector<std::string> output_node_names_;

    bool loadNormalizationParams(const std::string& jsonPath);
};
