#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QString>
#include <QImage>
#include <QQmlEngine>
#include <QPointF>
#include <atomic>
#include "GalleryModel.h"
#include "image/ImageEditor.h"
#include "OcrResultModel.h"
#include "ai/OCR.h"

class OcrRunnable;

class PhotoViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString ocrResults READ ocrResults WRITE setOcrResults NOTIFY ocrResultsChanged)
    Q_PROPERTY(OcrResultModel* ocrModel READ ocrModel CONSTANT)
    Q_PROPERTY(QStringList gifFrames READ gifFrames WRITE setGifFrames NOTIFY gifFramesChanged)
    Q_PROPERTY(GalleryModel* gallery READ gallery CONSTANT)
    Q_PROPERTY(ImageEditor* editor READ editor CONSTANT)
    Q_PROPERTY(bool isOcrLoading READ isOcrLoading NOTIFY ocrLoadingChanged)

public:
    explicit PhotoViewModel(QObject *parent = nullptr);
    ~PhotoViewModel();

    QString ocrResults() const { return m_ocrResults; }
    OcrResultModel* ocrModel() { return &m_ocrModel; }
    QStringList gifFrames() const { return m_gifFrames; }
    GalleryModel* gallery() { return &m_gallery; }
    ImageEditor* editor() { return &m_editor; }
    bool isOcrLoading() const { return m_isOcrLoading; }

    Q_INVOKABLE QPointF mapImageToFlickable(double x, double y, double imageWidth, double imageHeight,
        double paintedWidth, double paintedHeight, double xOffset, double yOffset, double zoomScale) const {
        return QPointF((x / imageWidth) * paintedWidth * zoomScale + xOffset,
                       (y / imageHeight) * paintedHeight * zoomScale + yOffset);
    }

    void updateOcrModel(const std::vector<OCRResult>& results) {
        m_ocrModel.setResults(results);
    }

    Q_INVOKABLE void extractSubject(const QString& sourceUrl);
    Q_INVOKABLE void runOcr(const QString& sourceUrl);
    Q_INVOKABLE void copyTextToClipboard(const QString& text);
    Q_INVOKABLE void copyImageToClipboard(const QString& sourceUrl);

    Q_INVOKABLE qint64 cacheSizeBytes() const;
    Q_INVOKABLE int cacheEntries();
    Q_INVOKABLE int cacheMaxEntries() const;
    Q_INVOKABLE void setCacheMaxEntries(int n);
    Q_INVOKABLE void clearCache();

public slots:
    void setOcrResults(const QString& results) {
        if (m_ocrResults != results) {
            m_ocrResults = results;
            emit ocrResultsChanged();
            emit aiTaskCompleted("OCR");
        }
    }
    void setGifFrames(const QStringList& frames) {
        if (m_gifFrames != frames) {
            m_gifFrames = frames;
            emit gifFramesChanged();
            emit aiTaskCompleted("GIF");
        }
    }

signals:
    void ocrResultsChanged();
    void gifFramesChanged();
    void subjectExtracted(const QString& subjectUrl);
    void aiTaskCompleted(const QString& taskName);
    void ocrLoadingChanged(bool loading);

private:
    QString m_ocrResults;
    OcrResultModel m_ocrModel;
    QStringList m_gifFrames;
    GalleryModel m_gallery;
    ImageEditor m_editor;

    QString m_currentToken;
    bool m_isOcrLoading = false;
    std::atomic<int> m_activeOcrTasks{0};

    friend class OcrRunnable;
};
