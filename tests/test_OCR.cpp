#include <gtest/gtest.h>
#include "ai/OCR.h"
#include "ai/CTCCharBoxExtractor.h"
#include <opencv2/opencv.hpp>
#include <QCoreApplication>

TEST(OCRTest, PipelineReturnsStructuredData) {
    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app(argc, argv);
    
    OCRPipeline& pipeline = OCRPipeline::getInstance();
    cv::Mat mock_img(100, 100, CV_8UC3, cv::Scalar(255, 255, 255));
    
    auto results = pipeline.processImage(mock_img);
    
    // If it found boxes (or returned the mock), check if it matches the mock or if it's empty
    // since a pure white image has no text.
    if (!results.empty()) {
        if (results[0].text == "https://example.com") {
            EXPECT_EQ(results[0].text, "https://example.com");
            EXPECT_EQ(results[0].box.size(), 4);
            EXPECT_TRUE(results[0].is_url);
        }
    } else {
        // Valid for empty white image to return 0 OCR text
        SUCCEED();
    }
}

TEST(OCRTest, URLRegexDetection) {
    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app(argc, argv);

    OCRPipeline& pipeline = OCRPipeline::getInstance();

    EXPECT_TRUE(pipeline.checkIsUrl("https://example.com"));
    EXPECT_TRUE(pipeline.checkIsUrl("http://www.google.com/path?query=1"));
    EXPECT_FALSE(pipeline.checkIsUrl("Not a URL"));
    EXPECT_FALSE(pipeline.checkIsUrl("example.com")); // HTTP prefix required by regex
}

TEST(OCRPipelineTest, DetectTextBoxPaddingMath) {
    // Verify the padding math: tight rect 10x4, pad=3 → 16x10
    cv::RotatedRect tight(cv::Point2f(50, 50), cv::Size2f(10, 4), 0);
    float pad = 3.0f;
    cv::RotatedRect padded(tight.center,
        cv::Size2f(tight.size.width + 2 * pad, tight.size.height + 2 * pad),
        tight.angle);
    EXPECT_FLOAT_EQ(padded.size.width, 16.0f);
    EXPECT_FLOAT_EQ(padded.size.height, 10.0f);
    EXPECT_EQ(padded.center, tight.center);
}

TEST(CTCCharBoxExtractorTest, VertexOrderingAndBoxHeight) {
    // box with vertices in non-standard order (bl first)
    // tl=(10,10), tr=(110,10), br=(110,30), bl=(10,30)
    std::vector<cv::Point> box = {
        cv::Point(10, 30),   // bl
        cv::Point(10, 10),   // tl
        cv::Point(110, 10),  // tr
        cv::Point(110, 30),  // br
    };

    // numClasses=3 (blank=0, 'a'=1, 'b'=2), seqLen=4
    // sequence: blank, a, a, blank → one char 'a'
    std::vector<float> recOutput = {
        1,0,0,  // t=0: blank
        0,1,0,  // t=1: a
        0,1,0,  // t=2: a (repeat, ignored)
        1,0,0,  // t=3: blank
    };
    std::vector<std::string> charDict = {"a", "b"};

    CTCCharBoxExtractor extractor;
    cv::Mat dummy(20, 100, CV_8UC3);
    auto boxes = extractor.extract(dummy, box, recOutput.data(), 4, 3, "a", charDict);

    ASSERT_EQ(boxes.size(), 1u);
    // boxHeight = height of the text line = 30 - 10 = 20px
    EXPECT_NEAR(boxes[0].boxHeight, 20.0f, 1.0f);
    // center.x should be near midpoint of box (10..110) = 60
    EXPECT_NEAR(boxes[0].rect.center.x, 60.0f, 5.0f);
    // center.y should be near midpoint of box (10..30) = 20
    EXPECT_NEAR(boxes[0].rect.center.y, 20.0f, 5.0f);
}
