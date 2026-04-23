#pragma once
#include <QString>
#include <QImage>
#include <vector>
#include <QImageReader>

class ImageIO {
public:
    ImageIO();
    ~ImageIO();
    
    // Load a static image
    QImage loadStaticImage(const QString& path);
    
    // Parse GIF frames
    std::vector<QImage> parseGifFrames(const QString& path);
};
