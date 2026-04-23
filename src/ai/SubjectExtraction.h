#pragma once

#include <opencv2/opencv.hpp>

class SubjectExtractionPipeline {
public:
    SubjectExtractionPipeline() = default;
    ~SubjectExtractionPipeline() = default;

    // Pipeline accepts an RGB image and returns an RGBA image where the background pixels have zero alpha value.
    cv::Mat extractSubject(const cv::Mat& rgbImage);

    // Steps
    cv::Mat preprocess(const cv::Mat& rgbImage);
    cv::Mat inferMask(const cv::Mat& preprocessedImage);
    cv::Mat applyMask(const cv::Mat& originalImage, const cv::Mat& mask);
};
