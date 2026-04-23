#pragma once
#include <QQuickAsyncImageProvider>
#include <QThreadPool>
#include <QImage>
#include <QString>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <map>
#include <mutex>

class AsyncImageResponse : public QQuickImageResponse, public QRunnable {
public:
    AsyncImageResponse(const QString &id, const QSize &requestedSize, const QImage& image)
        : m_id(id), m_requestedSize(requestedSize), m_image(image) {
        setAutoDelete(false);
    }

    void run() override {
        if (m_image.isNull()) {
            QString path = m_id;
            if (path.startsWith("//")) {
                path = path.mid(1);
            }
            if (!m_image.load(path)) {
                m_image = QImage(1, 1, QImage::Format_RGB32);
                m_image.fill(Qt::black);
            }
        }
        
        if (m_requestedSize.isValid() && !m_image.isNull()) {
            m_image = m_image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        emit finished();
    }

    QQuickTextureFactory *textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    QString m_id;
    QSize m_requestedSize;
    QImage m_image;
};

class AsyncImageProvider : public QQuickAsyncImageProvider {
public:
    static AsyncImageProvider* instance() {
        static AsyncImageProvider* inst = new AsyncImageProvider();
        return inst;
    }

    AsyncImageProvider() {}

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override {
        QImage image;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_images.find(id) != m_images.end()) {
                image = m_images.at(id);
            }
        }
        // If image is null (not found in map), it will be loaded from disk in run() based on id.

        AsyncImageResponse *response = new AsyncImageResponse(id, requestedSize, image);
        m_pool.start(response);
        return response;
    }


    void addImage(const QString& id, const cv::Mat& mat) {
        QImage image;
        if (mat.type() == CV_8UC3) {
            image = QImage((const uchar*) mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888).rgbSwapped().copy();
        } else if (mat.type() == CV_8UC1) {
            image = QImage((const uchar*) mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
        } else if (mat.type() == CV_8UC4) {
            image = QImage((const uchar*) mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGBA8888).rgbSwapped().copy();
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        m_images[id] = image;
    }
    
    void addImage(const QString& id, const QImage& image) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_images[id] = image;
    }

private:
    std::map<QString, QImage> m_images;
    std::mutex m_mutex;
    QThreadPool m_pool;
};
