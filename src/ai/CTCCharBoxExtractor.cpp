#include "CTCCharBoxExtractor.h"
#include <QString>
#include <algorithm>

std::vector<CharBox> CTCCharBoxExtractor::extract(
    const cv::Mat& cropImg,
    const std::vector<cv::Point>& box,
    const float* recOutput,
    int seqLen,
    int numClasses,
    const std::string& decodedText,
    const std::vector<std::string>& charDict
) {
    std::vector<CharBox> charBoxes;
    if (box.size() < 4 || decodedText.empty()) return charBoxes;

    struct CharSpan { int charIdx; int tStart; int tEnd; };
    std::vector<CharSpan> spans;
    int lastIdx = -1;
    QString qtext = QString::fromStdString(decodedText);
    int charPos = 0;

    for (int t = 0; t < seqLen; ++t) {
        int maxIdx = 0;
        float maxVal = -1e9f;
        for (int j = 0; j < numClasses; ++j) {
            float val = recOutput[t * numClasses + j];
            if (val > maxVal) { maxVal = val; maxIdx = j; }
        }
        if (maxIdx > 0 && maxIdx != lastIdx) {
            if (lastIdx > 0 && charPos > 0) {
                spans.back().tEnd = t - 1;
            }
            spans.push_back({charPos, t, t});
            charPos++;
        }
        if (maxIdx > 0 && !spans.empty()) {
            spans.back().tEnd = t;
        }
        lastIdx = maxIdx;
    }

    if (spans.empty()) return charBoxes;

    // Sort vertices to reliably identify tl/tr/bl/br regardless of minAreaRect order
    std::vector<cv::Point2f> pts = {
        cv::Point2f(box[0].x, box[0].y),
        cv::Point2f(box[1].x, box[1].y),
        cv::Point2f(box[2].x, box[2].y),
        cv::Point2f(box[3].x, box[3].y)
    };
    std::sort(pts.begin(), pts.end(), [](const cv::Point2f& a, const cv::Point2f& b) {
        return a.y < b.y;
    });
    if (pts[0].x > pts[1].x) std::swap(pts[0], pts[1]);
    if (pts[2].x > pts[3].x) std::swap(pts[2], pts[3]);
    cv::Point2f tl = pts[0], tr = pts[1], bl = pts[2], br = pts[3];

    float boxAngle = std::atan2(tr.y - tl.y, tr.x - tl.x);
    float boxHeight = (cv::norm(bl - tl) + cv::norm(br - tr)) / 2.0f;

    int qIdx = 0;
    for (size_t i = 0; i < spans.size() && qIdx < qtext.size(); ++i) {
        float ratioStart = (float)spans[i].tStart / seqLen;
        float ratioEnd = ((float)spans[i].tEnd + 1.0f) / seqLen;

        cv::Point2f topStart = tl + ratioStart * (tr - tl);
        cv::Point2f topEnd = tl + ratioEnd * (tr - tl);
        cv::Point2f botStart = bl + ratioStart * (br - bl);
        cv::Point2f botEnd = bl + ratioEnd * (br - bl);

        cv::Point2f center = (topStart + topEnd + botStart + botEnd) * 0.25f;
        float charWidth = (cv::norm(topEnd - topStart) + cv::norm(botEnd - botStart)) / 2.0f;

        CharBox cb;
        cb.ch = qtext.at(qIdx);
        cb.rect = cv::RotatedRect(center, cv::Size2f(charWidth, boxHeight), boxAngle * 180.0f / CV_PI);
        cb.boxHeight = boxHeight;
        charBoxes.push_back(cb);
        qIdx++;
    }

    return charBoxes;
}
