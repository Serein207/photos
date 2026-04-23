#include "ImageEditing.h"
#include <stdexcept>
#include <cmath>

namespace ImageEditing {

cv::Mat crop(const cv::Mat& image, const cv::Rect& roi) {
    if (roi.x < 0 || roi.y < 0 || roi.x + roi.width > image.cols || roi.y + roi.height > image.rows) {
        throw std::invalid_argument("Crop ROI is out of bounds");
    }
    return image(roi); // non-destructive crop creates a new header pointing to the same data
}

cv::Mat rotate90(const cv::Mat& image, int degrees) {
    degrees = degrees % 360;
    if (degrees < 0) degrees += 360;

    cv::Mat result;
    if (degrees == 90) {
        cv::rotate(image, result, cv::ROTATE_90_CLOCKWISE);
    } else if (degrees == 180) {
        cv::rotate(image, result, cv::ROTATE_180);
    } else if (degrees == 270) {
        cv::rotate(image, result, cv::ROTATE_90_COUNTERCLOCKWISE);
    } else if (degrees == 0) {
        result = image.clone();
    } else {
        throw std::invalid_argument("Only 90-degree rotations are supported");
    }
    return result;
}

void drawBrushPath(cv::Mat& image, const std::vector<cv::Point>& points, const cv::Scalar& color, int thickness) {
    if (points.size() < 2) return;
    
    // Draw the path as a polyline
    const cv::Point* pts[1] = { points.data() };
    int npts[1] = { static_cast<int>(points.size()) };
    
    cv::polylines(image, pts, npts, 1, false, color, thickness, cv::LINE_AA);
}

void drawRectangle(cv::Mat& image, cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness) {
    cv::rectangle(image, pt1, pt2, color, thickness, cv::LINE_AA);
}

void drawEllipse(cv::Mat& image, cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness) {
    cv::Point center((pt1.x + pt2.x) / 2, (pt1.y + pt2.y) / 2);
    cv::Size axes(std::abs(pt1.x - pt2.x) / 2, std::abs(pt1.y - pt2.y) / 2);
    cv::ellipse(image, center, axes, 0, 0, 360, color, thickness, cv::LINE_AA);
}

void drawArrow(cv::Mat& image, cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness) {
    cv::arrowedLine(image, pt1, pt2, color, thickness, cv::LINE_AA, 0, 0.1);
}

} // namespace ImageEditing
