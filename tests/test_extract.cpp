#include <opencv2/opencv.hpp>
#include "../src/SubjectExtraction.h"
#include <iostream>

int main() {
    cv::Mat img = cv::Mat::zeros(100, 100, CV_8UC3);
    SubjectExtractionPipeline pipeline;
    cv::Mat rgba = pipeline.extractSubject(img);
    if(rgba.empty()) { std::cout << "Empty\n"; return 1; }
    
    int non_zero_alpha = 0;
    int max_alpha = 0;
    for(int i=0; i<rgba.rows; i++) {
        for(int j=0; j<rgba.cols; j++) {
            uchar a = rgba.at<cv::Vec4b>(i, j)[3];
            if(a > 0) non_zero_alpha++;
            if(a > max_alpha) max_alpha = a;
        }
    }
    std::cout << "Non zero alpha: " << non_zero_alpha << " Max alpha: " << max_alpha << "\n";
    return 0;
}
