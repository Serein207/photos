#pragma once

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

class AIEngine {
public:
    AIEngine(const std::string& model_path);
    ~AIEngine();

    // Tensor conversion utilities
    static Ort::Value MatToTensor(const cv::Mat& mat, const std::vector<int64_t>& shape, Ort::MemoryInfo& memory_info);
    static cv::Mat TensorToMat(Ort::Value& tensor, int rows, int cols, int type);

    Ort::Session* getSession() const { return session_.get(); }
    Ort::Env* getEnv() const { return env_.get(); }

private:
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
};
