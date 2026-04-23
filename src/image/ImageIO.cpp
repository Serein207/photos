#include "ImageIO.h"

ImageIO::ImageIO() {}
ImageIO::~ImageIO() {}

QImage ImageIO::loadStaticImage(const QString& path) {
    QImage image(path);
    return image;
}

std::vector<QImage> ImageIO::parseGifFrames(const QString& path) {
    std::vector<QImage> frames;
    QImageReader reader(path);
    if (!reader.canRead()) return frames;

    int imageCount = reader.imageCount();
    if (imageCount <= 0) {
        while (reader.canRead()) {
            frames.push_back(reader.read());
        }
    } else {
        for (int i = 0; i < imageCount; ++i) {
            reader.jumpToImage(i);
            frames.push_back(reader.read());
        }
    }
    return frames;
}
