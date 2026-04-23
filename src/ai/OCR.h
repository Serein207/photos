#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <QChar>

struct CharBox {
    QChar ch;
    cv::RotatedRect rect;
    float boxHeight;  // text-line height, unaffected by RotatedRect axis swap
};

class CharBoxExtractor {
public:
    virtual ~CharBoxExtractor() = default;
    virtual std::vector<CharBox> extract(
        const cv::Mat& cropImg,
        const std::vector<cv::Point>& box,
        const float* recOutput,
        int seqLen,
        int numClasses,
        const std::string& decodedText,
        const std::vector<std::string>& charDict
    ) = 0;
};

struct OCRResult {
    std::string text;
    std::vector<cv::Point> box;
    std::vector<CharBox> chars;
    bool is_url;
};

struct RecognitionResult {
    std::string text;
    std::vector<CharBox> chars;
};

class OCRPipeline {
public:
    static OCRPipeline& getInstance() {
        static OCRPipeline instance;
        return instance;
    }

    ~OCRPipeline() = default;

    OCRPipeline(const OCRPipeline&) = delete;
    OCRPipeline& operator=(const OCRPipeline&) = delete;

    std::vector<OCRResult> processImage(const cv::Mat& image);

    // Signal the current processImage to abort at the next checkpoint.
    void cancel() { m_cancelled.store(true, std::memory_order_relaxed); }

    std::vector<std::vector<cv::Point>> detectText(const cv::Mat& image);
    RecognitionResult recognizeText(const cv::Mat& image, const std::vector<cv::Point>& box);
    bool checkIsUrl(const std::string& text);

    void setCharBoxExtractor(std::unique_ptr<CharBoxExtractor> extractor) {
        m_charBoxExtractor = std::move(extractor);
    }

private:
    OCRPipeline();
    Ort::Env env;
    Ort::SessionOptions sessionOptions;
    std::unique_ptr<Ort::Session> detSession;
    std::unique_ptr<Ort::Session> recSession;
    std::vector<std::string> m_charDict;
    std::mutex m_mutex;
    std::atomic<bool> m_cancelled{false};
    std::unique_ptr<CharBoxExtractor> m_charBoxExtractor;
};
