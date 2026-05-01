#pragma once

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <memory>
#include <mutex>
#include <string>

class AIEngine;

class SAMPipeline {
public:
    SAMPipeline(const std::string& encoderPath, const std::string& decoderPath);
    ~SAMPipeline();

    bool isModelLoaded() const;

    // Takes BGR image, returns single-channel float mask (0.0~1.0)
    cv::Mat inferMask(const cv::Mat& bgrImage);

private:
    cv::Mat preprocessImage(const cv::Mat& bgrImage, cv::Size& resizedSize,
                            cv::Mat& transformMatrix);
    Ort::Value runEncoder(const cv::Mat& preprocessedImage);
    cv::Mat runDecoder(Ort::Value& imageEmbeddings,
                       const cv::Size& originalSize,
                       const cv::Mat& transformMatrix);
    cv::Mat postprocessMask(const cv::Mat& mask,
                            const cv::Size& originalSize,
                            const cv::Mat& transformMatrix);

    std::unique_ptr<AIEngine> encoderEngine_;
    std::unique_ptr<AIEngine> decoderEngine_;
    std::mutex inferMutex_;
    static constexpr int TARGET_SIZE = 1024;
    static constexpr int ENCODER_HEIGHT = 684;
    static constexpr int ENCODER_WIDTH = 1024;
};
