#pragma once

#include <QObject>
#include <QStringList>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QQmlEngine>

class GalleryModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QStringList galleryImages READ galleryImages NOTIFY galleryImagesChanged)

public:
    explicit GalleryModel(QObject *parent = nullptr);
    QStringList galleryImages() const { return m_galleryImages; }
    Q_INVOKABLE void loadImages();

signals:
    void galleryImagesChanged();

private:
    QStringList m_galleryImages;
    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
};
