#include <QtTest>
#include <QElapsedTimer>
#include <QDebug>
#include <opencv2/opencv.hpp>
#include <functional>

#include "ai/OCR.h"
#include "ai/SubjectExtraction.h"
#include "ai/SAMPipeline.h"

class TestBenchmark : public QObject {
    Q_OBJECT

    struct Result {
        QString name;
        double ms;
        int iter;
    };
    QVector<Result> m_results;

    void record(const QString& name, std::function<void()> fn, int iter = 3) {
        double total = 0;
        for (int i = 0; i < iter; i++) {
            QElapsedTimer t; t.start();
            fn();
            total += t.nsecsElapsed() / 1e6;
        }
        double avg = total / iter;
        m_results.append({name, avg, iter});
        qDebug().noquote() << QString("  %1  %2 ms (n=%3)")
            .arg(name, -42).arg(avg, 8, 'f', 1).arg(iter);
    }

    cv::Mat makeImage(int w, int h) {
        cv::Mat img(h, w, CV_8UC3, cv::Scalar(255,255,255));
        for (int r = 0; r < 5; r++) {
            for (int c = 0; c < 3; c++) {
                int x = 20 + c*(w/3), y = 20 + r*(h/5);
                int bw = w/4, bh = h/7;
                if (x+bw < w && y+bh < h) {
                    cv::Rect roi(x, y, bw, bh);
                    cv::rectangle(img, roi, cv::Scalar(240,240,240), -1);
                    cv::rectangle(img, roi, cv::Scalar(200,200,200), 1);
                    cv::putText(img, "Test 测试", cv::Point(x+5, y+bh/2),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(50,50,50), 1);
                }
            }
        }
        return img;
    }

    cv::Mat makeObjImage(int w, int h) {
        cv::Mat img(h, w, CV_8UC3, cv::Scalar(200,220,240));
        int cx = w/2, cy = h/2, r = std::min(w,h)/4;
        cv::circle(img, cv::Point(cx,cy), r, cv::Scalar(50,100,200), -1);
        return img;
    }

private slots:
    void initTestCase() {
        qDebug() << "\n========== Performance Benchmark ==========";
    }

    void benchImageIO() {
        qDebug() << "\n--- Image Loading ---";
        record("JPEG encode+decode 4032x3024", []() {
            cv::Mat m(3024,4032,CV_8UC3);
            cv::randu(m, cv::Scalar::all(0), cv::Scalar::all(255));
            std::vector<uchar> buf;
            cv::imencode(".jpg", m, buf);
            cv::imdecode(buf, cv::IMREAD_COLOR);
        }, 5);

        record("PNG encode+decode 1920x1080", []() {
            cv::Mat m(1080,1920,CV_8UC3);
            cv::randu(m, cv::Scalar::all(0), cv::Scalar::all(255));
            std::vector<uchar> buf;
            cv::imencode(".png", m, buf);
            cv::imdecode(buf, cv::IMREAD_COLOR);
        }, 5);

        record("Resize 4032->1920", []() {
            cv::Mat s(3024,4032,CV_8UC3), d;
            cv::resize(s, d, cv::Size(1920,1080));
        }, 10);

        record("cv::Mat -> QImage", []() {
            cv::Mat s(1080,1920,CV_8UC3);
            cv::randu(s, cv::Scalar::all(0), cv::Scalar::all(255));
            QImage q(s.data, s.cols, s.rows, s.step, QImage::Format_RGB888);
            QImage c = q.rgbSwapped().copy();
        }, 10);
    }

    void benchEdit() {
        qDebug() << "\n--- Image Editing ---";
        record("Rotate 90deg 4032x3024", []() {
            cv::Mat s(3024,4032,CV_8UC3), d;
            cv::rotate(s, d, cv::ROTATE_90_CLOCKWISE);
        }, 10);

        record("Crop 50% 4032x3024", []() {
            cv::Mat s(3024,4032,CV_8UC3);
            cv::Mat c = s(cv::Rect(1008,756,2016,1512));
        }, 10);

        record("GaussianBlur 1920x1080", []() {
            cv::Mat s(1080,1920,CV_8UC3), d;
            cv::GaussianBlur(s, d, cv::Size(5,5), 0);
        }, 10);
    }

    void benchOCR() {
        qDebug() << "\n--- OCR Pipeline ---";
        auto& pipe = OCRPipeline::getInstance();

        record("OCR detectText 800x600", [&]() {
            pipe.detectText(makeImage(800,600));
        }, 3);

        record("OCR detectText 1920x1080", [&]() {
            pipe.detectText(makeImage(1920,1080));
        }, 3);

        record("OCR detectText 4032x3024", [&]() {
            pipe.detectText(makeImage(4032,3024));
        }, 2);

        record("OCR full pipeline 800x600", [&]() {
            pipe.processImage(makeImage(800,600));
        }, 2);

        record("OCR full pipeline 1920x1080", [&]() {
            pipe.processImage(makeImage(1920,1080));
        }, 2);

        record("OCR full pipeline 4032x3024", [&]() {
            pipe.processImage(makeImage(4032,3024));
        }, 1);
    }

    void benchSubject() {
        qDebug() << "\n--- Subject Extraction ---";
        SubjectExtractionPipeline pipe;

        record("SubjectExtract 800x600", [&]() {
            pipe.extractSubject(makeObjImage(800,600));
        }, 2);

        record("SubjectExtract 1920x1080", [&]() {
            pipe.extractSubject(makeObjImage(1920,1080));
        }, 2);

        record("SubjectExtract 4032x3024", [&]() {
            pipe.extractSubject(makeObjImage(4032,3024));
        }, 1);
    }

    void cleanupTestCase() {
        qDebug() << "\n========== Summary ==========";
        qDebug() << QString("%1  %2  %3")
            .arg("Test", -45).arg("Avg(ms)", 9).arg("Iter");
        qDebug() << QString(62, '-');
        for (auto& r : m_results)
            qDebug().noquote() << QString("%1  %2  %3")
                .arg(r.name, -45).arg(r.ms, 8, 'f', 1).arg(r.iter, 4);

        qDebug() << "\n========== Thesis Table Data ==========";
        for (auto& r : m_results)
            qDebug().noquote()
                << QString("| %1 | %2 ms | n=%3 |")
                   .arg(r.name).arg(r.ms, 0, 'f', 1).arg(r.iter);
    }
};

QTEST_MAIN(TestBenchmark)
#include "test_Benchmark.moc"
