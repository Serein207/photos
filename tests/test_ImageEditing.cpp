#include <QtTest>
#include <opencv2/opencv.hpp>
#include "image/ImageEditing.h"

class TestImageEditing : public QObject {
    Q_OBJECT

private slots:
    void testRotate90();
    void testCrop();
    void testDrawBrushPath();
    void testDrawShapes();
};

void TestImageEditing::testRotate90() {
    cv::Mat image = cv::Mat::zeros(100, 200, CV_8UC3); // 100 rows, 200 cols
    
    cv::Mat rotated90 = ImageEditing::rotate90(image, 90);
    QCOMPARE(rotated90.rows, 200);
    QCOMPARE(rotated90.cols, 100);

    cv::Mat rotated180 = ImageEditing::rotate90(image, 180);
    QCOMPARE(rotated180.rows, 100);
    QCOMPARE(rotated180.cols, 200);

    cv::Mat rotated270 = ImageEditing::rotate90(image, -90);
    QCOMPARE(rotated270.rows, 200);
    QCOMPARE(rotated270.cols, 100);
}

void TestImageEditing::testCrop() {
    cv::Mat image = cv::Mat::zeros(100, 200, CV_8UC3); // 100 rows, 200 cols
    cv::Rect roi(10, 20, 50, 60); // x=10, y=20, w=50, h=60

    cv::Mat cropped = ImageEditing::crop(image, roi);
    
    QCOMPARE(cropped.rows, 60);
    QCOMPARE(cropped.cols, 50);
}

void TestImageEditing::testDrawBrushPath() {
    cv::Mat image = cv::Mat::zeros(100, 100, CV_8UC3);
    
    std::vector<cv::Point> points = {
        cv::Point(10, 10),
        cv::Point(20, 20),
        cv::Point(30, 20)
    };
    
    cv::Scalar color(0, 0, 255); // Red (BGR format in OpenCV)
    int thickness = 2;
    
    ImageEditing::drawBrushPath(image, points, color, thickness);
    
    // Check if a point on the line is painted
    // Note: polylines with anti-aliasing can spread the color, but the exact point should be colored
    cv::Vec3b pixel = image.at<cv::Vec3b>(10, 10);
    QVERIFY(pixel[2] > 0); // R channel should have some value
    
    pixel = image.at<cv::Vec3b>(20, 20);
    QVERIFY(pixel[2] > 0);
    
    pixel = image.at<cv::Vec3b>(20, 30); // x=30, y=20
    QVERIFY(pixel[2] > 0);
}

void TestImageEditing::testDrawShapes() {
    cv::Mat image = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::Scalar color(0, 255, 0); // Green
    
    // Rectangle
    ImageEditing::drawRectangle(image, cv::Point(10, 10), cv::Point(30, 30), color, 2);
    QVERIFY(image.at<cv::Vec3b>(10, 20)[1] > 0); // top edge
    
    // Ellipse (bounding box 30,40 to 70,60 -> center 50,50, axes 20,10)
    ImageEditing::drawEllipse(image, cv::Point(30, 40), cv::Point(70, 60), color, 2);
    QVERIFY(image.at<cv::Vec3b>(50, 70)[1] > 0); // right edge
    
    // Arrow
    ImageEditing::drawArrow(image, cv::Point(10, 80), cv::Point(40, 80), color, 2);
    QVERIFY(image.at<cv::Vec3b>(80, 25)[1] > 0); // middle of arrow
}

QTEST_MAIN(TestImageEditing)
#include "test_ImageEditing.moc"
