#include "SubjectExtraction.h"
#include "SAMPipeline.h"
#include <QCoreApplication>
#include <QDir>

SubjectExtractionPipeline::SubjectExtractionPipeline() {}

SubjectExtractionPipeline::~SubjectExtractionPipeline() = default;

SAMPipeline& SubjectExtractionPipeline::samPipeline() {
    static SAMPipeline pipeline = []() {
        QString appDir = QCoreApplication::applicationDirPath();
        QString encoderPath = QDir(appDir).filePath("../assets/mobile_sam_encoder.onnx");
        QString decoderPath = QDir(appDir).filePath("../assets/mobile_sam_decoder.onnx");
        return SAMPipeline(encoderPath.toStdString(), decoderPath.toStdString());
    }();
    return pipeline;
}

bool SubjectExtractionPipeline::isSAMAvailable() const {
    return samPipeline().isModelLoaded();
}

cv::Mat SubjectExtractionPipeline::extractSubject(const cv::Mat& rgbImage) {
    if (rgbImage.empty() || rgbImage.channels() != 3) {
        return cv::Mat();
    }

    if (isSAMAvailable()) {
        cv::Mat bgrImage;
        cv::cvtColor(rgbImage, bgrImage, cv::COLOR_RGB2BGR);
        cv::Mat mask = samPipeline().inferMask(bgrImage);
        if (!mask.empty()) {
            return applyMask(rgbImage, mask);
        }
    }

    return extractWithGrabCut(rgbImage);
}

cv::Mat SubjectExtractionPipeline::extractWithGrabCut(const cv::Mat& rgbImage) {
    int max_dim = 512;
    double scale = 1.0;
    cv::Mat smallImage;
    if (rgbImage.cols > max_dim || rgbImage.rows > max_dim) {
        scale = (double)max_dim / std::max(rgbImage.cols, rgbImage.rows);
        cv::resize(rgbImage, smallImage, cv::Size(), scale, scale, cv::INTER_AREA);
    } else {
        smallImage = rgbImage;
    }

    cv::Mat mask = cv::Mat::zeros(smallImage.size(), CV_8UC1);
    cv::Mat bgdModel, fgdModel;

    cv::Rect rect(
        smallImage.cols * 0.05,
        smallImage.rows * 0.05,
        smallImage.cols * 0.90,
        smallImage.rows * 0.90
    );

    cv::grabCut(smallImage, mask, rect, bgdModel, fgdModel, 5, cv::GC_INIT_WITH_RECT);

    cv::Mat fgMask = (mask == cv::GC_PR_FGD) | (mask == cv::GC_FGD);

    cv::Mat resizedMask;
    if (scale != 1.0) {
        cv::resize(fgMask, resizedMask, rgbImage.size(), 0, 0, cv::INTER_LINEAR);
    } else {
        resizedMask = fgMask;
    }

    cv::Mat smoothMask;
    cv::GaussianBlur(resizedMask, smoothMask, cv::Size(5, 5), 0);

    cv::Mat mask32F;
    smoothMask.convertTo(mask32F, CV_32FC1, 1.0 / 255.0);

    return applyMask(rgbImage, mask32F);
}

cv::Mat SubjectExtractionPipeline::applyMask(const cv::Mat& originalImage, const cv::Mat& mask) {
    if (originalImage.channels() != 3) {
        return cv::Mat();
    }

    cv::Mat mask8U;
    cv::Mat clampedMask;
    cv::min(cv::max(mask, 0.0f), 1.0f, clampedMask);
    clampedMask.convertTo(mask8U, CV_8UC1, 255.0);

    cv::Mat rgba;
    cv::cvtColor(originalImage, rgba, cv::COLOR_RGB2BGRA);

    std::vector<cv::Mat> channels;
    cv::split(rgba, channels);
    channels[3] = mask8U;
    cv::merge(channels, rgba);
    return rgba;
}
