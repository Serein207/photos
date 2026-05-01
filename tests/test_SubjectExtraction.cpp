#include <QtTest>
#include <QImage>
#include <opencv2/opencv.hpp>
#include "ai/SubjectExtraction.h"

class TestSubjectExtraction : public QObject {
    Q_OBJECT

private slots:
    void testExtractionPipeline();
    void testEmptyInput();
};

void TestSubjectExtraction::testExtractionPipeline() {
    SubjectExtractionPipeline pipeline;

    cv::Mat rgbImage(600, 800, CV_8UC3, cv::Scalar(100, 150, 200));
    cv::Mat rgbaImage = pipeline.extractSubject(rgbImage);

    QVERIFY(!rgbaImage.empty());
    QCOMPARE(rgbaImage.channels(), 4);
    QCOMPARE(rgbaImage.rows, rgbImage.rows);
    QCOMPARE(rgbaImage.cols, rgbImage.cols);
}

void TestSubjectExtraction::testEmptyInput() {
    SubjectExtractionPipeline pipeline;

    cv::Mat empty;
    cv::Mat result = pipeline.extractSubject(empty);
    QVERIFY(result.empty());
}

QTEST_MAIN(TestSubjectExtraction)
#include "test_SubjectExtraction.moc"
