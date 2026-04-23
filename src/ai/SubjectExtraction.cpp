#include "SubjectExtraction.h"

cv::Mat SubjectExtractionPipeline::extractSubject(const cv::Mat& rgbImage) {
    if (rgbImage.empty() || rgbImage.channels() != 3) {
        return cv::Mat();
    }

    // 1. Resize to a smaller image for GrabCut speed
    int max_dim = 512;
    double scale = 1.0;
    cv::Mat smallImage;
    if (rgbImage.cols > max_dim || rgbImage.rows > max_dim) {
        scale = (double)max_dim / std::max(rgbImage.cols, rgbImage.rows);
        cv::resize(rgbImage, smallImage, cv::Size(), scale, scale, cv::INTER_AREA);
    } else {
        smallImage = rgbImage;
    }

    // 2. GrabCut
    cv::Mat mask = cv::Mat::zeros(smallImage.size(), CV_8UC1);
    cv::Mat bgdModel, fgdModel;
    
    // Rect initialized slightly inward
    cv::Rect rect(
        smallImage.cols * 0.05, 
        smallImage.rows * 0.05, 
        smallImage.cols * 0.90, 
        smallImage.rows * 0.90
    );
    
    cv::grabCut(smallImage, mask, rect, bgdModel, fgdModel, 5, cv::GC_INIT_WITH_RECT);
    
    // 3. Extract foreground mask (pixels marked as foreground or probable foreground)
    cv::Mat fgMask = (mask == cv::GC_PR_FGD) | (mask == cv::GC_FGD);

    // 4. Resize mask back to original size
    cv::Mat resizedMask;
    if (scale != 1.0) {
        cv::resize(fgMask, resizedMask, rgbImage.size(), 0, 0, cv::INTER_LINEAR);
    } else {
        resizedMask = fgMask;
    }

    // 5. Refine mask edges slightly (optional but good)
    cv::Mat smoothMask;
    cv::GaussianBlur(resizedMask, smoothMask, cv::Size(5, 5), 0);

    // Normalize mask to 0.0 - 1.0 for applyMask
    cv::Mat mask32F;
    smoothMask.convertTo(mask32F, CV_32FC1, 1.0 / 255.0);

    return applyMask(rgbImage, mask32F);
}

cv::Mat SubjectExtractionPipeline::preprocess(const cv::Mat& rgbImage) {
    // Unused, maintained for API compatibility
    return rgbImage;
}

cv::Mat SubjectExtractionPipeline::inferMask(const cv::Mat& preprocessedImage) {
    // Unused, maintained for API compatibility
    return cv::Mat();
}

cv::Mat SubjectExtractionPipeline::applyMask(const cv::Mat& originalImage, const cv::Mat& mask) {
    // 3. Apply mask to original image to create a transparent RGBA cutout
    cv::Mat rgba;
    if (originalImage.channels() != 3) {
        return cv::Mat();
    }

    cv::Mat mask8U;
    cv::Mat clampedMask;
    cv::min(cv::max(mask, 0.0f), 1.0f, clampedMask);
    clampedMask.convertTo(mask8U, CV_8UC1, 255.0);

    // Create BGRA image (since OpenCV uses BGR natively, and AsyncImageProvider handles the swap)
    cv::cvtColor(originalImage, rgba, cv::COLOR_RGB2BGRA);

    // Split channels to assign alpha
    std::vector<cv::Mat> channels;
    cv::split(rgba, channels);
    
    // Assign mask to alpha channel
    channels[3] = mask8U;
    
    cv::merge(channels, rgba);
    return rgba;
}
