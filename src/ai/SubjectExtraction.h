#pragma once

#include <opencv2/opencv.hpp>

class SAMPipeline;

class SubjectExtractionPipeline {
public:
    SubjectExtractionPipeline();
    ~SubjectExtractionPipeline();

    cv::Mat extractSubject(const cv::Mat& rgbImage);
    bool isSAMAvailable() const;

private:
    static SAMPipeline& samPipeline();

    cv::Mat extractWithGrabCut(const cv::Mat& rgbImage);
    cv::Mat applyMask(const cv::Mat& originalImage, const cv::Mat& mask);
};
