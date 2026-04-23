#include <gtest/gtest.h>
#include "ai/TextLayoutAnalyzer.h"

TEST(TextLayoutAnalyzerTest, SingleLineLeftToRight) {
    std::vector<OCRResult> results;
    results.push_back({"world", {cv::Point(200,50), cv::Point(300,50), cv::Point(300,80), cv::Point(200,80)}, {}, false});
    results.push_back({"hello", {cv::Point(50,50), cv::Point(150,50), cv::Point(150,80), cv::Point(50,80)}, {}, false});

    auto ordered = TextLayoutAnalyzer::analyze(results);
    ASSERT_EQ(ordered.size(), 2);
    EXPECT_EQ(ordered[0].originalIndex, 1); // "hello" first
    EXPECT_EQ(ordered[1].originalIndex, 0); // "world" second
    EXPECT_EQ(ordered[0].lineIndex, ordered[1].lineIndex);
}

TEST(TextLayoutAnalyzerTest, TwoLinesTopToBottom) {
    std::vector<OCRResult> results;
    results.push_back({"line2", {cv::Point(50,100), cv::Point(150,100), cv::Point(150,130), cv::Point(50,130)}, {}, false});
    results.push_back({"line1", {cv::Point(50,50), cv::Point(150,50), cv::Point(150,80), cv::Point(50,80)}, {}, false});

    auto ordered = TextLayoutAnalyzer::analyze(results);
    ASSERT_EQ(ordered.size(), 2);
    EXPECT_EQ(ordered[0].originalIndex, 1); // "line1" first (top)
    EXPECT_EQ(ordered[1].originalIndex, 0); // "line2" second (bottom)
}

TEST(TextLayoutAnalyzerTest, TwoColumns) {
    std::vector<OCRResult> results;
    results.push_back({"L1", {cv::Point(50,50), cv::Point(150,50), cv::Point(150,80), cv::Point(50,80)}, {}, false});
    results.push_back({"L2", {cv::Point(50,100), cv::Point(150,100), cv::Point(150,130), cv::Point(50,130)}, {}, false});
    results.push_back({"R1", {cv::Point(400,50), cv::Point(500,50), cv::Point(500,80), cv::Point(400,80)}, {}, false});
    results.push_back({"R2", {cv::Point(400,100), cv::Point(500,100), cv::Point(500,130), cv::Point(400,130)}, {}, false});

    auto ordered = TextLayoutAnalyzer::analyze(results);
    ASSERT_EQ(ordered.size(), 4);
    EXPECT_EQ(ordered[0].originalIndex, 0); // L1
    EXPECT_EQ(ordered[1].originalIndex, 1); // L2
    EXPECT_EQ(ordered[2].originalIndex, 2); // R1
    EXPECT_EQ(ordered[3].originalIndex, 3); // R2
}

TEST(TextLayoutAnalyzerTest, EmptyInput) {
    auto ordered = TextLayoutAnalyzer::analyze({});
    EXPECT_TRUE(ordered.empty());
}
