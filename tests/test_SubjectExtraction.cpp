#include <QtTest>
#include <QImage>
#include <opencv2/opencv.hpp>
#include "ai/SubjectExtraction.h"

class TestSubjectExtraction : public QObject {
    Q_OBJECT

private slots:
    void testExtractionPipeline();
};

void TestSubjectExtraction::testExtractionPipeline() {
    SubjectExtractionPipeline pipeline;

    // Create a 800x600 RGB image
    cv::Mat rgbImage(600, 800, CV_8UC3, cv::Scalar(100, 150, 200));

    // Run pipeline
    cv::Mat rgbaImage = pipeline.extractSubject(rgbImage);

    // Verify it accepts RGB and returns RGBA
    QVERIFY(!rgbaImage.empty());
    QCOMPARE(rgbaImage.channels(), 4);
    QCOMPARE(rgbaImage.rows, rgbImage.rows);
    QCOMPARE(rgbaImage.cols, rgbImage.cols);

    // Verify background pixels have zero alpha value and subject has 255
    // The mask center has 255, edges have 0
    // Check center pixel
    cv::Vec4b centerPixel = rgbaImage.at<cv::Vec4b>(300, 400);
    QCOMPARE(centerPixel[3], 255);

    // Check corner pixel (background)
    cv::Vec4b cornerPixel = rgbaImage.at<cv::Vec4b>(0, 0);
    QCOMPARE(cornerPixel[3], 0);
}

QTEST_MAIN(TestSubjectExtraction)
#include "test_SubjectExtraction.moc"
