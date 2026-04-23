#include <gtest/gtest.h>
#include "ai/AIEngine.h"
#include <opencv2/opencv.hpp>
#include <vector>

TEST(AIEngineTest, InitializeEngine) {
    // We might not have a model path, so we can't test session initialization successfully without throwing an exception or returning error, 
    // unless we create a dummy model or handle it.
    // The requirement is "AIEngine initializes without crashing".
    EXPECT_NO_THROW({
        AIEngine engine("non_existent_model.onnx");
    });
}

TEST(AIEngineTest, MatToTensorConversion) {
    cv::Mat mat(100, 100, CV_32FC3, cv::Scalar(0.5f, 0.5f, 0.5f));
    std::vector<int64_t> shape = {1, 100, 100, 3};
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    
    Ort::Value tensor = AIEngine::MatToTensor(mat, shape, memory_info);
    
    EXPECT_TRUE(tensor.IsTensor());
    
    auto tensor_info = tensor.GetTensorTypeAndShapeInfo();
    EXPECT_EQ(tensor_info.GetShape(), shape);
    EXPECT_EQ(tensor_info.GetElementType(), ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
}

TEST(AIEngineTest, TensorToMatConversion) {
    std::vector<int64_t> shape = {1, 100, 100, 3};
    std::vector<float> data(100 * 100 * 3, 0.5f);
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    
    Ort::Value tensor = Ort::Value::CreateTensor<float>(memory_info, data.data(), data.size(), shape.data(), shape.size());
    
    cv::Mat mat = AIEngine::TensorToMat(tensor, 100, 100, CV_32FC3);
    
    EXPECT_EQ(mat.rows, 100);
    EXPECT_EQ(mat.cols, 100);
    EXPECT_EQ(mat.type(), CV_32FC3);
    EXPECT_FLOAT_EQ(mat.at<cv::Vec3f>(0, 0)[0], 0.5f);
}
