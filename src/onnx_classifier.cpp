#include "onnx_classifier.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>

// Вспомогательная функция для применения Softmax
void applySoftmax(std::array<float, 3>& scores) {
    float max_val = scores[0];
    for (int i = 1; i < 3; ++i) {
        if (scores[i] > max_val) max_val = scores[i];
    }
    
    float sum_exp = 0.0f;
    for (int i = 0; i < 3; ++i) {
        scores[i] = std::exp(scores[i] - max_val);
        sum_exp += scores[i];
    }
    
    for (int i = 0; i < 3; ++i) {
        scores[i] /= sum_exp;
    }
}

ONNXClassifier::ONNXClassifier() : env_{nullptr} {
    const OrtApi* api = OrtGetApiBase()->GetApi(17);
    OrtEnv* c_env;
    api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "ONNXClassifier", &c_env);
    env_ = Ort::Env(c_env);
}

bool ONNXClassifier::loadNormalizationParams(const std::string& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть JSON файл: " << jsonPath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    auto extractArray = [&content](const std::string& key, std::vector<double>& out_array) -> bool {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = content.find(searchKey);
        if (keyPos == std::string::npos) return false;
        
        size_t startBracket = content.find('[', keyPos);
        size_t endBracket = content.find(']', startBracket);
        if (startBracket == std::string::npos || endBracket == std::string::npos) return false;
        
        std::string arrayStr = content.substr(startBracket + 1, endBracket - startBracket - 1);
        
        size_t pos = 0;
        while (pos < arrayStr.length()) {
            size_t startNum = arrayStr.find_first_not_of(" \t\n\r,", pos);
            if (startNum == std::string::npos) break;
            
            size_t endNum = arrayStr.find_first_of(" \t\n\r,", startNum);
            if (endNum == std::string::npos) endNum = arrayStr.length();
            
            std::string numStr = arrayStr.substr(startNum, endNum - startNum);
            try {
                out_array.push_back(std::stod(numStr));
            } catch (...) {}
            pos = endNum;
        }
        return !out_array.empty();
    };

    mean_.clear();
    std_.clear();
    bool meanOk = extractArray("mean", mean_);
    bool stdOk = extractArray("std", std_);

    if (!meanOk || !stdOk || mean_.size() != 6 || std_.size() != 6) {
        std::cerr << "Ошибка парсинга JSON: массивы mean и std некорректны!" << std::endl;
        return false;
    }

    return true;
}

bool ONNXClassifier::initialize(const std::string& modelPath, const std::string& jsonPath) {
    if (!loadNormalizationParams(jsonPath)) {
        return false;
    }

    try {
        std::wstring wModelPath(modelPath.begin(), modelPath.end());
        
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        session_ = Ort::Session(env_, wModelPath.c_str(), session_options);

        size_t num_input_nodes = session_.GetInputCount();
        for (size_t i = 0; i < num_input_nodes; i++) {
        #if ORT_API_VERSION >= 14
            Ort::AllocatedStringPtr input_name_ptr = session_.GetInputNameAllocated(i, allocator_);
            input_node_names_.push_back(input_name_ptr.get());
        #else
            char* input_name = session_.GetInputName(i, allocator_);
            input_node_names_.push_back(input_name);
            allocator_.Free(input_name);
        #endif
        }

        size_t num_output_nodes = session_.GetOutputCount();
        for (size_t i = 0; i < num_output_nodes; i++) {
        #if ORT_API_VERSION >= 14
            Ort::AllocatedStringPtr output_name_ptr = session_.GetOutputNameAllocated(i, allocator_);
            output_node_names_.push_back(output_name_ptr.get());
        #else
            char* output_name = session_.GetOutputName(i, allocator_);
            output_node_names_.push_back(output_name);
            allocator_.Free(output_name);
        #endif
        }
        
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Init Error: " << e.what() << std::endl;
        return false;
    }
}

ClassificationResult ONNXClassifier::predict(const std::array<float, 6>& raw_features) {
    if (!session_) {
        throw std::runtime_error("Attempted to predict, but ONNX model is not loaded. Call initialize() first.");
    }

    std::vector<float> input_tensor_values(6);
    for (size_t i = 0; i < 6; ++i) {
        input_tensor_values[i] = static_cast<float>((raw_features[i] - mean_[i]) / std_[i]);
    }

    std::vector<int64_t> input_shape = {1, 6};
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, 
        input_tensor_values.data(), 
        input_tensor_values.size(), 
        input_shape.data(), 
        input_shape.size()
    );

    std::vector<const char*> input_names_ptrs;
    for (const auto& name : input_node_names_) input_names_ptrs.push_back(name.c_str());

    std::vector<const char*> output_names_ptrs;
    for (const auto& name : output_node_names_) output_names_ptrs.push_back(name.c_str());

    auto output_tensors = session_.Run(
        Ort::RunOptions{nullptr}, 
        input_names_ptrs.data(), 
        &input_tensor, 
        1, 
        output_names_ptrs.data(), 
        output_names_ptrs.size()
    );

    float* floatarr = output_tensors.front().GetTensorMutableData<float>();
    std::array<float, 3> scores = {floatarr[0], floatarr[1], floatarr[2]};

    applySoftmax(scores);

    int best_label = 0;
    float max_confidence = scores[0];
    
    for (int i = 1; i < 3; ++i) {
        if (scores[i] > max_confidence) {
            max_confidence = scores[i];
            best_label = i;
        }
    }

    return ClassificationResult {
        best_label,
        max_confidence,
        scores
    };
}
