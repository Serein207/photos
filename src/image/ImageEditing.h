#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

namespace ImageEditing {

// Non-destructive crop: just returns a submatrix (ROI)
cv::Mat crop(const cv::Mat& image, const cv::Rect& roi);

// 90-degree rotations. degrees must be 90, 180, 270, -90, -180, -270
cv::Mat rotate90(const cv::Mat& image, int degrees);

// Polyline drawing for brush tool
// points: sequence of points forming the path
// color: brush color (BGR)
// thickness: brush size
void drawBrushPath(cv::Mat& image, const std::vector<cv::Point>& points, const cv::Scalar& color, int thickness = 2);

// Shape drawing functions
void drawRectangle(cv::Mat& image, cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness = 2);
void drawEllipse(cv::Mat& image, cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness = 2);
void drawArrow(cv::Mat& image, cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness = 2);

} // namespace ImageEditing
