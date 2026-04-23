#include "PhotoViewModel.h"
#include "util/OcrCache.h"
#include <QFile>
#include <QCryptographicHash>
#include <QThreadPool>
#include <QRunnable>
#include <QStandardPaths>
#include <QUuid>
#include <QPointer>
#include "ai/SubjectExtraction.h"
#include "image/AsyncImageProvider.h"
#include <QGuiApplication>
#include <QClipboard>

PhotoViewModel::PhotoViewModel(QObject *parent) : QObject(parent) {}

PhotoViewModel::~PhotoViewModel() {
    // Wait for all in-flight OCR runnables to finish before destroying members.
    QThreadPool::globalInstance()->waitForDone();
}

// ---------- helpers ----------

namespace {

QString computeMd5(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash h(QCryptographicHash::Md5);
    h.addData(&f);
    return h.result().toHex();
}

} // namespace

class OcrRunnable : public QRunnable {
public:
    OcrRunnable(QPointer<PhotoViewModel> vm, QString token, std::string path)
        : m_vm(vm), m_token(std::move(token)), m_path(std::move(path))
    { setAutoDelete(true); }

    void run() override {
        ++m_vm->m_activeOcrTasks;

        cv::Mat image = cv::imread(m_path, cv::IMREAD_COLOR);
        std::vector<OCRResult> results;
        if (!image.empty()) {
            cv::Mat rgb;
            cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
            results = OCRPipeline::getInstance().processImage(rgb);
        }

        OcrCache::instance().store(m_token, results);

        QString token = m_token;
        std::vector<OCRResult> res = results;
        QMetaObject::invokeMethod(m_vm.data(), [vm = m_vm, token, res]() {
            if (!vm) return;
            --vm->m_activeOcrTasks;
            if (vm->m_currentToken == token) {
                vm->updateOcrModel(res);
                vm->m_isOcrLoading = false;
                emit vm->ocrLoadingChanged(false);
            }
        }, Qt::QueuedConnection);
    }

private:
    QPointer<PhotoViewModel> m_vm;
    QString m_token;
    std::string m_path;
};

// ---------- runOcr ----------

void PhotoViewModel::runOcr(const QString& sourceUrl) {
    QString path = sourceUrl;
    if (path.startsWith("image://photo_provider/"))
        path = path.mid(23);
    else if (path.startsWith("file://"))
        path = path.mid(7);

    if (path.isEmpty()) {
        m_currentToken.clear();
        m_isOcrLoading = false;
        emit ocrLoadingChanged(false);
        updateOcrModel({});
        return;
    }

    QString token = computeMd5(path);
    if (token.isEmpty()) return;

    if (auto cached = OcrCache::instance().lookup(token)) {
        m_currentToken = token;
        updateOcrModel(*cached);
        return;
    }

    m_currentToken = token;
    m_isOcrLoading = true;
    emit ocrLoadingChanged(true);
    OCRPipeline::getInstance().cancel();
    QThreadPool::globalInstance()->start(
        new OcrRunnable(QPointer<PhotoViewModel>(this), token, path.toStdString())
    );
}

// ---------- extractSubject ----------

void PhotoViewModel::extractSubject(const QString& sourceUrl) {
    QString path = sourceUrl;
    if (path.startsWith("image://photo_provider/"))
        path = path.mid(23);
    else if (path.startsWith("file://"))
        path = path.mid(7);

    std::string stdPath = path.toStdString();
    QPointer<PhotoViewModel> ptr(this);

    std::thread([ptr, stdPath]() {
        cv::Mat image = cv::imread(stdPath, cv::IMREAD_COLOR);
        if (!image.empty()) {
            cv::Mat rgbImage;
            cv::cvtColor(image, rgbImage, cv::COLOR_BGR2RGB);
            SubjectExtractionPipeline pipeline;
            cv::Mat rgba = pipeline.extractSubject(rgbImage);
            if (!rgba.empty()) {
                QString uniqueId = "subject_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
                AsyncImageProvider::instance()->addImage(uniqueId, rgba);
                QString resultUrl = "image://photo_provider/" + uniqueId;
                QImage qimg = QImage((const uchar*)rgba.data, rgba.cols, rgba.rows, rgba.step,
                                     QImage::Format_RGBA8888).rgbSwapped().copy();
                if (ptr) {
                    QMetaObject::invokeMethod(ptr.data(), [ptr, resultUrl, qimg]() {
                        if (ptr) {
                            emit ptr->subjectExtracted(resultUrl);
                            emit ptr->aiTaskCompleted("SubjectExtraction");
                            if (QClipboard *cb = QGuiApplication::clipboard())
                                cb->setImage(qimg);
                        }
                    }, Qt::QueuedConnection);
                }
            }
        }
    }).detach();
}

// ---------- clipboard ----------

void PhotoViewModel::copyTextToClipboard(const QString& text) {
    if (QClipboard *cb = QGuiApplication::clipboard())
        cb->setText(text);
}

void PhotoViewModel::copyImageToClipboard(const QString& sourceUrl) {
    QString path = sourceUrl;
    if (path.startsWith("image://photo_provider/")) path = path.mid(23);
    else if (path.startsWith("file://")) path = path.mid(7);

    cv::Mat image = cv::imread(path.toStdString(), cv::IMREAD_UNCHANGED);
    if (image.empty()) return;
    QImage qimg;
    if (image.channels() == 4) {
        cv::Mat bgra; cv::cvtColor(image, bgra, cv::COLOR_BGRA2RGBA);
        qimg = QImage((const uchar*)bgra.data, bgra.cols, bgra.rows, bgra.step, QImage::Format_RGBA8888).copy();
    } else if (image.channels() == 3) {
        cv::Mat rgb; cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
        qimg = QImage((const uchar*)rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    } else if (image.channels() == 1) {
        qimg = QImage((const uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_Grayscale8).copy();
    }
    if (!qimg.isNull())
        if (QClipboard *cb = QGuiApplication::clipboard())
            cb->setImage(qimg);
}

// ---------- cache management ----------

qint64 PhotoViewModel::cacheSizeBytes() const {
    return OcrCache::instance().cacheSize();
}

int PhotoViewModel::cacheEntries() {
    return OcrCache::instance().cacheEntries();
}

int PhotoViewModel::cacheMaxEntries() const {
    return OcrCache::instance().maxEntries();
}

void PhotoViewModel::setCacheMaxEntries(int n) {
    OcrCache::instance().setMaxEntries(n);
}

void PhotoViewModel::clearCache() {
    OcrCache::instance().clearCache();
}
