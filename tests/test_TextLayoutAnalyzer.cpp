#include <QtTest>
#include "ai/TextLayoutAnalyzer.h"

class TestTextLayoutAnalyzer : public QObject {
    Q_OBJECT

private slots:
    void testSingleLineLeftToRight();
    void testTwoLinesTopToBottom();
    void testTwoColumns();
    void testEmptyInput();
};

void TestTextLayoutAnalyzer::testSingleLineLeftToRight() {
    std::vector<OCRResult> results;
    results.push_back({"world", {cv::Point(200,50), cv::Point(300,50), cv::Point(300,80), cv::Point(200,80)}, {}, false});
    results.push_back({"hello", {cv::Point(50,50), cv::Point(150,50), cv::Point(150,80), cv::Point(50,80)}, {}, false});

    auto ordered = TextLayoutAnalyzer::analyze(results);
    QCOMPARE(ordered.size(), size_t(2));
    QCOMPARE(ordered[0].originalIndex, 1);
    QCOMPARE(ordered[1].originalIndex, 0);
}

void TestTextLayoutAnalyzer::testTwoLinesTopToBottom() {
    std::vector<OCRResult> results;
    results.push_back({"line2", {cv::Point(50,100), cv::Point(150,100), cv::Point(150,130), cv::Point(50,130)}, {}, false});
    results.push_back({"line1", {cv::Point(50,50), cv::Point(150,50), cv::Point(150,80), cv::Point(50,80)}, {}, false});

    auto ordered = TextLayoutAnalyzer::analyze(results);
    QCOMPARE(ordered.size(), size_t(2));
    QCOMPARE(ordered[0].originalIndex, 1);
    QCOMPARE(ordered[1].originalIndex, 0);
}

void TestTextLayoutAnalyzer::testTwoColumns() {
    std::vector<OCRResult> results;
    results.push_back({"L1", {cv::Point(50,50), cv::Point(150,50), cv::Point(150,80), cv::Point(50,80)}, {}, false});
    results.push_back({"L2", {cv::Point(50,100), cv::Point(150,100), cv::Point(150,130), cv::Point(50,130)}, {}, false});
    results.push_back({"R1", {cv::Point(400,50), cv::Point(500,50), cv::Point(500,80), cv::Point(400,80)}, {}, false});
    results.push_back({"R2", {cv::Point(400,100), cv::Point(500,100), cv::Point(500,130), cv::Point(400,130)}, {}, false});

    auto ordered = TextLayoutAnalyzer::analyze(results);
    QCOMPARE(ordered.size(), size_t(4));
    QCOMPARE(ordered[0].originalIndex, 0);
    QCOMPARE(ordered[1].originalIndex, 1);
    QCOMPARE(ordered[2].originalIndex, 2);
    QCOMPARE(ordered[3].originalIndex, 3);
}

void TestTextLayoutAnalyzer::testEmptyInput() {
    auto ordered = TextLayoutAnalyzer::analyze({});
    QVERIFY(ordered.empty());
}

QTEST_MAIN(TestTextLayoutAnalyzer)
#include "test_TextLayoutAnalyzer.moc"
