#include <QImage>
#include <iostream>

int main() {
    uchar data[4] = {255, 0, 0, 128}; // R, G, B, A
    QImage img(data, 1, 1, QImage::Format_RGBA8888);
    QImage swapped = img.rgbSwapped();
    std::cout << "Swapped format: " << swapped.format() << "\n";
    QRgb p = swapped.pixel(0, 0);
    std::cout << "A: " << qAlpha(p) << " R: " << qRed(p) << " G: " << qGreen(p) << " B: " << qBlue(p) << "\n";
    return 0;
}
