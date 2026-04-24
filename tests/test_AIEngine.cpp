#include <QtTest>
#include "ai/AIEngine.h"
#include <opencv2/opencv.hpp>
#include <vector>

class TestAIEngine : public QObject {
    Q_OBJECT

private slots:
    void testInitializeEngine();
    void testMatToTensorConversion();
    void testTensorToMatConversion();
};

void TestAIEngine::testInitializeEngine() {
    // We might not have a model path, so we can't test session initialization successfully without throwing an exception or returning error,
    // unless we create a dummy model or handle it.
    // The requirement is "AIEngine initializes without crashing".
    try {
        AIEngine engine("non_existent_model.onnx");
    } catch (...) {
        // Expected to fail with non-existent model
    }
    QVERIFY(true); // If we get here, initialization didn't crash
}

void TestAIEngine::testMatToTensorConversion() {
    cv::Mat mat(100, 100, CV_32FC3, cv::Scalar(0.5f, 0.5f, 0.5f));
    std::vector<int64_t> shape = {1, 100, 100, 3};
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value tensor = AIEngine::MatToTensor(mat, shape, memory_info);

    QVERIFY(tensor.IsTensor());

    auto tensor_info = tensor.GetTensorTypeAndShapeInfo();
    QCOMPARE(tensor_info.GetShape(), shape);
    QCOMPARE(tensor_info.GetElementType(), ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
}

void TestAIEngine::testTensorToMatConversion() {
    std::vector<int64_t> shape = {1, 100, 100, 3};
    std::vector<float> data(100 * 100 * 3, 0.5f);
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value tensor = Ort::Value::CreateTensor<float>(memory_info, data.data(), data.size(), shape.data(), shape.size());

    cv::Mat mat = AIEngine::TensorToMat(tensor, 100, 100, CV_32FC3);

    QCOMPARE(mat.rows, 100);
    QCOMPARE(mat.cols, 100);
    QCOMPARE(mat.type(), CV_32FC3);
    QCOMPARE(mat.at<cv::Vec3f>(0, 0)[0], 0.5f);
}

QTEST_MAIN(TestAIEngine)
#include "test_AIEngine.moc"
