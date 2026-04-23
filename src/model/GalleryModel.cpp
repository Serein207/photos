#include "GalleryModel.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfoList>

GalleryModel::GalleryModel(QObject *parent) : QObject(parent) {
    m_reloadTimer.setSingleShot(true);
    m_reloadTimer.setInterval(500);

    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir dir(picturesPath);
    if (!dir.exists()) dir.mkpath(".");
    m_watcher.addPath(picturesPath);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        m_reloadTimer.start();
    });
    connect(&m_reloadTimer, &QTimer::timeout, this, &GalleryModel::loadImages);
    loadImages();
}

void GalleryModel::loadImages() {
    m_galleryImages.clear();
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir dir(picturesPath);
    if (dir.exists()) {
        QStringList nameFilters;
        nameFilters << "*.jpg" << "*.jpeg" << "*.png" << "*.gif" << "*.bmp" << "*.webp";
        QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
        for (const QFileInfo& fileInfo : fileList)
            m_galleryImages.append("image://photo_provider/" + fileInfo.absoluteFilePath());
    }
    emit galleryImagesChanged();
}
