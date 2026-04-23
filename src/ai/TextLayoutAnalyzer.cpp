#include "TextLayoutAnalyzer.h"
#include <algorithm>
#include <cmath>

namespace {

struct BoxInfo {
    int index;
    cv::Point2f center;
    float angle;
    float height;
    float width;
};

float boxAngle(const std::vector<cv::Point>& box) {
    if (box.size() < 4) return 0.0f;
    float dx = box[1].x - box[0].x;
    float dy = box[1].y - box[0].y;
    return std::atan2(dy, dx) * 180.0f / CV_PI;
}

cv::Point2f boxCenter(const std::vector<cv::Point>& box) {
    cv::Point2f c(0, 0);
    for (auto& p : box) { c.x += p.x; c.y += p.y; }
    if (!box.empty()) { c.x /= box.size(); c.y /= box.size(); }
    return c;
}

float boxWidth(const std::vector<cv::Point>& box) {
    if (box.size() < 4) return 0.0f;
    float w1 = cv::norm(cv::Point2f(box[1].x - box[0].x, box[1].y - box[0].y));
    float w2 = cv::norm(cv::Point2f(box[2].x - box[3].x, box[2].y - box[3].y));
    return (w1 + w2) / 2.0f;
}

float boxHeight(const std::vector<cv::Point>& box) {
    if (box.size() < 4) return 0.0f;
    float h1 = cv::norm(cv::Point2f(box[3].x - box[0].x, box[3].y - box[0].y));
    float h2 = cv::norm(cv::Point2f(box[2].x - box[1].x, box[2].y - box[1].y));
    return (h1 + h2) / 2.0f;
}

struct Line {
    std::vector<int> indices;
    float avgY;
};

} // namespace

std::vector<OrderedResult> TextLayoutAnalyzer::analyze(const std::vector<OCRResult>& results) {
    if (results.empty()) return {};

    std::vector<BoxInfo> infos;
    for (int i = 0; i < (int)results.size(); ++i) {
        BoxInfo bi;
        bi.index = i;
        bi.center = boxCenter(results[i].box);
        bi.angle = boxAngle(results[i].box);
        bi.height = boxHeight(results[i].box);
        bi.width = boxWidth(results[i].box);
        infos.push_back(bi);
    }

    // Group by angle (±15 degrees)
    std::vector<bool> assigned(infos.size(), false);
    std::vector<std::vector<int>> angleGroups;

    for (size_t i = 0; i < infos.size(); ++i) {
        if (assigned[i]) continue;
        std::vector<int> group = {(int)i};
        assigned[i] = true;
        for (size_t j = i + 1; j < infos.size(); ++j) {
            if (assigned[j]) continue;
            float diff = std::abs(infos[i].angle - infos[j].angle);
            if (diff > 180) diff = 360 - diff;
            if (diff <= 15.0f) {
                group.push_back((int)j);
                assigned[j] = true;
            }
        }
        angleGroups.push_back(group);
    }

    // Sort groups: horizontal-ish first (smallest absolute angle)
    std::sort(angleGroups.begin(), angleGroups.end(), [&](const auto& a, const auto& b) {
        float aa = std::abs(infos[a[0]].angle);
        float ab = std::abs(infos[b[0]].angle);
        return aa < ab;
    });

    int lineIdx = 0;
    std::vector<OrderedResult> output;

    for (auto& group : angleGroups) {
        float avgHeight = 0;
        for (int idx : group) avgHeight += infos[idx].height;
        avgHeight /= group.size();
        float lineThreshold = avgHeight * 0.6f;

        // Sort by Y first
        std::sort(group.begin(), group.end(), [&](int a, int b) {
            return infos[a].center.y < infos[b].center.y;
        });

        // Cluster into lines by Y proximity — join nearest line within threshold
        std::vector<Line> lines;
        for (int idx : group) {
            float bestDist = lineThreshold;
            Line* bestLine = nullptr;
            for (auto& line : lines) {
                float dist = std::abs(infos[idx].center.y - line.avgY);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestLine = &line;
                }
            }
            if (bestLine) {
                bestLine->indices.push_back(idx);
                int n = (int)bestLine->indices.size();
                bestLine->avgY = bestLine->avgY + (infos[idx].center.y - bestLine->avgY) / n;
            } else {
                lines.push_back({{idx}, infos[idx].center.y});
            }
        }

        // Sort each line by X
        for (auto& line : lines) {
            std::sort(line.indices.begin(), line.indices.end(), [&](int a, int b) {
                return infos[a].center.x < infos[b].center.x;
            });
        }

        // Detect columns: per-row largest-gap voting
        std::vector<float> candidateXs;
        for (auto& line : lines) {
            if (line.indices.size() < 2) continue;
            float maxGap = 0;
            float maxGapMid = 0;
            for (size_t k = 1; k < line.indices.size(); ++k) {
                int prev = line.indices[k - 1];
                int cur  = line.indices[k];
                float rightEdge = infos[prev].center.x + infos[prev].width / 2.0f;
                float leftEdge  = infos[cur].center.x  - infos[cur].width  / 2.0f;
                float gap = leftEdge - rightEdge;
                if (gap > maxGap) {
                    maxGap = gap;
                    maxGapMid = (rightEdge + leftEdge) / 2.0f;
                }
            }
            if (maxGap > avgHeight * 2.0f)
                candidateXs.push_back(maxGapMid);
        }

        // Cluster candidate X positions within avgHeight * 1.0
        std::sort(candidateXs.begin(), candidateXs.end());
        std::vector<std::pair<float, int>> clusters;
        for (float x : candidateXs) {
            bool merged = false;
            for (auto& cl : clusters) {
                if (std::abs(x - cl.first) < avgHeight * 1.0f) {
                    cl.first = (cl.first * cl.second + x) / (cl.second + 1);
                    cl.second++;
                    merged = true;
                    break;
                }
            }
            if (!merged) clusters.push_back({x, 1});
        }

        // Keep only clusters with majority vote.
        // When there is only 1 line, a single vote is enough (the gap is unambiguous).
        int voteThreshold = (lines.size() == 1) ? 1 : std::max(2, ((int)lines.size() + 1) / 2);
        std::vector<float> colSplits;
        for (auto& cl : clusters) {
            if (cl.second >= voteThreshold)
                colSplits.push_back(cl.first);
        }
        std::sort(colSplits.begin(), colSplits.end());

        int numCols = (int)colSplits.size() + 1;
        std::vector<std::vector<std::vector<int>>> colLines(numCols);
        for (auto& line : lines) {
            std::vector<std::vector<int>> splitLine(numCols);
            for (int idx : line.indices) {
                int col = 0;
                for (size_t s = 0; s < colSplits.size(); ++s) {
                    if (infos[idx].center.x > colSplits[s]) col = (int)s + 1;
                }
                splitLine[col].push_back(idx);
            }
            for (int c = 0; c < numCols; ++c) {
                if (!splitLine[c].empty())
                    colLines[c].push_back(splitLine[c]);
            }
        }

        // Output: columns left-to-right, lines top-to-bottom within each column.
        // Each OCR box gets its own unique lineIndex.
        for (int c = 0; c < numCols; ++c) {
            std::sort(colLines[c].begin(), colLines[c].end(), [&](const auto& a, const auto& b) {
                return infos[a[0]].center.y < infos[b[0]].center.y;
            });
            for (auto& line : colLines[c]) {
                for (int idx : line)
                    output.push_back({infos[idx].index, lineIdx++});
            }
        }
    }

    return output;
}
