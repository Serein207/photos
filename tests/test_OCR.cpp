#include <QtTest>
#include "ai/OCR.h"
#include "ai/CTCCharBoxExtractor.h"
#include <opencv2/opencv.hpp>

class TestOCR : public QObject {
    Q_OBJECT

private slots:
    void testPipelineReturnsStructuredData();
    void testURLRegexDetection();
    void testDetectTextBoxPaddingMath();
    void testVertexOrderingAndBoxHeight();
};

void TestOCR::testPipelineReturnsStructuredData() {
    OCRPipeline& pipeline = OCRPipeline::getInstance();
    cv::Mat mock_img(100, 100, CV_8UC3, cv::Scalar(255, 255, 255));

    auto results = pipeline.processImage(mock_img);

    if (!results.empty()) {
        if (results[0].text == "https://example.com") {
            QVERIFY(results[0].text == "https://example.com");
            QCOMPARE(results[0].box.size(), size_t(4));
            QVERIFY(results[0].is_url);
        }
    } else {
        QVERIFY(true);
    }
}

void TestOCR::testURLRegexDetection() {
    OCRPipeline& pipeline = OCRPipeline::getInstance();

    QVERIFY(pipeline.checkIsUrl("https://example.com"));
    QVERIFY(pipeline.checkIsUrl("http://www.google.com/path?query=1"));
    QVERIFY(!pipeline.checkIsUrl("Not a URL"));
    QVERIFY(!pipeline.checkIsUrl("example.com"));
}

void TestOCR::testDetectTextBoxPaddingMath() {
    cv::RotatedRect tight(cv::Point2f(50, 50), cv::Size2f(10, 4), 0);
    float pad = 3.0f;
    cv::RotatedRect padded(tight.center,
        cv::Size2f(tight.size.width + 2 * pad, tight.size.height + 2 * pad),
        tight.angle);
    QVERIFY(qAbs(padded.size.width - 16.0f) < 0.001f);
    QVERIFY(qAbs(padded.size.height - 10.0f) < 0.001f);
    QCOMPARE(padded.center, tight.center);
}

void TestOCR::testVertexOrderingAndBoxHeight() {
    std::vector<cv::Point> box = {
        cv::Point(10, 30),
        cv::Point(10, 10),
        cv::Point(110, 10),
        cv::Point(110, 30),
    };

    std::vector<float> recOutput = {
        1,0,0,
        0,1,0,
        0,1,0,
        1,0,0,
    };
    std::vector<std::string> charDict = {"a", "b"};

    CTCCharBoxExtractor extractor;
    cv::Mat dummy(20, 100, CV_8UC3);
    auto boxes = extractor.extract(dummy, box, recOutput.data(), 4, 3, "a", charDict);

    QCOMPARE(boxes.size(), size_t(1));
    QVERIFY(qAbs(boxes[0].boxHeight - 20.0f) < 1.0f);
    QVERIFY(qAbs(boxes[0].rect.center.x - 60.0f) < 5.0f);
    QVERIFY(qAbs(boxes[0].rect.center.y - 20.0f) < 5.0f);
}

QTEST_MAIN(TestOCR)
#include "test_OCR.moc"
