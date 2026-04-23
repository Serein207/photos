#include "AIEngine.h"
#include <iostream>
#include <stdexcept>

AIEngine::AIEngine(const std::string& model_path) {
    try {
        env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "AIEngine");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        
        session_ = std::make_unique<Ort::Session>(*env_, model_path.c_str(), session_options);
    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime initialization failed: " << e.what() << std::endl;
    }
}

AIEngine::~AIEngine() {}

Ort::Value AIEngine::MatToTensor(const cv::Mat& mat, const std::vector<int64_t>& shape, Ort::MemoryInfo& memory_info) {
    if (!mat.isContinuous()) {
        throw std::runtime_error("OpenCV Mat must be continuous for Tensor conversion.");
    }
    
    ONNXTensorElementDataType element_type = ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
    if (mat.depth() == CV_8U) {
        element_type = ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8;
    } else if (mat.depth() == CV_32F) {
        element_type = ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
    } else {
        throw std::runtime_error("Unsupported cv::Mat depth for Tensor conversion.");
    }

    size_t input_tensor_size = mat.total() * mat.elemSize();
    
    return Ort::Value::CreateTensor(memory_info,
                                    const_cast<void*>(reinterpret_cast<const void*>(mat.data)),
                                    input_tensor_size,
                                    shape.data(),
                                    shape.size(),
                                    element_type);
}

cv::Mat AIEngine::TensorToMat(Ort::Value& tensor, int rows, int cols, int type) {
    auto tensor_info = tensor.GetTensorTypeAndShapeInfo();
    ONNXTensorElementDataType element_type = tensor_info.GetElementType();

    if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT && CV_MAT_DEPTH(type) != CV_32F) {
        throw std::runtime_error("Tensor is FLOAT but cv::Mat depth is not CV_32F.");
    }
    if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8 && CV_MAT_DEPTH(type) != CV_8U) {
        throw std::runtime_error("Tensor is UINT8 but cv::Mat depth is not CV_8U.");
    }

    void* tensor_data = tensor.GetTensorMutableData<void>();
    return cv::Mat(rows, cols, type, tensor_data);
}
