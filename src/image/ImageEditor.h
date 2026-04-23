#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QQmlEngine>

class ImageEditor : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ImageEditor(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString applyRotation(const QString& sourceUrl, int angle);
    Q_INVOKABLE QString applyCrop(const QString& sourceUrl, double x, double y, double width, double height);
    Q_INVOKABLE QString applyBrush(const QString& sourceUrl, const QVariantList& points, const QString& color, int thickness);

private:
    static QString resolveImagePath(const QString& sourceUrl);
};
