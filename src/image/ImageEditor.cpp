#include "ImageEditor.h"
#include "ImageEditing.h"
#include "AsyncImageProvider.h"
#include <QStandardPaths>
#include <QUuid>
#include <QColor>
#include <QPointF>
#include <opencv2/opencv.hpp>

QString ImageEditor::resolveImagePath(const QString& sourceUrl) {
    QString path = sourceUrl;
    if (path.startsWith("image://photo_provider/"))
        path = path.mid(23);
    else if (path.startsWith("file://"))
        path = path.mid(7);
    return path;
}

QString ImageEditor::applyRotation(const QString& sourceUrl, int angle) {
    QString path = resolveImagePath(sourceUrl);
    cv::Mat image = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (image.empty()) return sourceUrl;

    cv::Mat result = ImageEditing::rotate90(image, angle);
    QString uniqueId = "edit_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".jpg";
    QString newPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + uniqueId;
    cv::imwrite(newPath.toStdString(), result);
    AsyncImageProvider::instance()->addImage(newPath, result);
    return "image://photo_provider/" + newPath;
}

QString ImageEditor::applyCrop(const QString& sourceUrl, double x, double y, double width, double height) {
    QString path = resolveImagePath(sourceUrl);
    cv::Mat image = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (image.empty()) return sourceUrl;

    cv::Rect roi(x, y, width, height);
    roi &= cv::Rect(0, 0, image.cols, image.rows);
    if (roi.width <= 0 || roi.height <= 0) return sourceUrl;

    cv::Mat result = ImageEditing::crop(image, roi);
    QString uniqueId = "edit_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".jpg";
    QString newPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + uniqueId;
    cv::imwrite(newPath.toStdString(), result);
    AsyncImageProvider::instance()->addImage(newPath, result);
    return "image://photo_provider/" + newPath;
}

QString ImageEditor::applyBrush(const QString& sourceUrl, const QVariantList& pointsList, const QString& colorHex, int thickness) {
    QString path = resolveImagePath(sourceUrl);
    cv::Mat image = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (image.empty()) return sourceUrl;

    std::vector<cv::Point> pts;
    for (const QVariant& v : pointsList) {
        QPointF pt = v.toPointF();
        pts.push_back(cv::Point(pt.x(), pt.y()));
    }
    QColor qc(colorHex);
    cv::Scalar color(qc.blue(), qc.green(), qc.red());
    cv::Mat result = image.clone();
    ImageEditing::drawBrushPath(result, pts, color, thickness);

    QString uniqueId = "edit_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".jpg";
    QString newPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + uniqueId;
    cv::imwrite(newPath.toStdString(), result);
    AsyncImageProvider::instance()->addImage(newPath, result);
    return "image://photo_provider/" + newPath;
}
