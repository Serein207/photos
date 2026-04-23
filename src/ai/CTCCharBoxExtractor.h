#pragma once

#include "OCR.h"

class CTCCharBoxExtractor : public CharBoxExtractor {
public:
    std::vector<CharBox> extract(
        const cv::Mat& cropImg,
        const std::vector<cv::Point>& box,
        const float* recOutput,
        int seqLen,
        int numClasses,
        const std::string& decodedText,
        const std::vector<std::string>& charDict
    ) override;
};
