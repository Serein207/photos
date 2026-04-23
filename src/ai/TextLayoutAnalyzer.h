#pragma once

#include "OCR.h"
#include <vector>

struct OrderedResult {
    int originalIndex;
    int lineIndex;  // globally unique across all lines in the image
};

class TextLayoutAnalyzer {
public:
    static std::vector<OrderedResult> analyze(const std::vector<OCRResult>& results);
};
