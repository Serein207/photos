#include <QtTest>
#include <QImage>
#include <QTemporaryDir>
#include <QFile>
#include "image/ImageIO.h"

class TestImageIO : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testLoadStaticImage();
    void testParseGifFrames();

private:
    QTemporaryDir m_tempDir;
    QString m_staticImagePath;
    QString m_gifImagePath;
};

void TestImageIO::initTestCase() {
    QVERIFY(m_tempDir.isValid());
    m_staticImagePath = m_tempDir.path() + "/test.png";
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::red);
    img.save(m_staticImagePath);

    m_gifImagePath = m_tempDir.path() + "/test.gif";
    QFile file(m_gifImagePath);
    if (file.open(QIODevice::WriteOnly)) {
        // A tiny 1x1 transparent GIF
        const char gifData[] = {
            'G', 'I', 'F', '8', '9', 'a', 
            1, 0, 1, 0, 
            (char)0x80, 0, 0, 
            0, 0, 0, 
            (char)0xff, (char)0xff, (char)0xff, 
            0x21, (char)0xf9, 4, 1, 0, 0, 0, 0, 
            0x2c, 0, 0, 0, 0, 1, 0, 1, 0, 0, 
            2, 2, 0x44, 1, 0, 
            0x3b
        };
        file.write(gifData, sizeof(gifData));
        file.close();
    }
}

void TestImageIO::testLoadStaticImage() {
    ImageIO io;
    QImage loaded = io.loadStaticImage(m_staticImagePath);
    QVERIFY(!loaded.isNull());
    QCOMPARE(loaded.width(), 100);
}

void TestImageIO::testParseGifFrames() {
    ImageIO io;
    auto frames = io.parseGifFrames(m_gifImagePath);
    QVERIFY(!frames.empty());
    QVERIFY(!frames[0].isNull());
}

QTEST_MAIN(TestImageIO)
#include "test_ImageIO.moc"
