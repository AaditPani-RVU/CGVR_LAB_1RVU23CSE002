#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kWidth = 800;
constexpr int kHeight = 600;

struct Point { int x; int y; };
struct Color { unsigned char red; unsigned char green; unsigned char blue; };

std::vector<Point> bresenhamLine(int x0, int y0, int x1, int y1) {
    std::vector<Point> pixels;
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    while (true) {
        pixels.push_back({x0, y0});
        if (x0 == x1 && y0 == y1) break;
        const int twiceError = 2 * error;
        if (twiceError >= dy) { error += dy; x0 += sx; }
        if (twiceError <= dx) { error += dx; y0 += sy; }
    }
    return pixels;
}

class Raster {
public:
    Raster() : pixels_(static_cast<std::size_t>(kWidth) * kHeight, {6, 12, 28}) {}

    void setPixel(int x, int y, Color color) {
        if (x >= 0 && x < kWidth && y >= 0 && y < kHeight)
            pixels_[static_cast<std::size_t>(y) * kWidth + x] = color;
    }
    void block(int x, int y, int radius, Color color) {
        for (int row = y - radius; row <= y + radius; ++row)
            for (int col = x - radius; col <= x + radius; ++col) setPixel(col, row, color);
    }
    bool write(const std::string& path) const {
        std::ofstream output(path, std::ios::binary);
        if (!output) return false;
        output << "P6\n" << kWidth << ' ' << kHeight << "\n255\n";
        for (const Color& pixel : pixels_) output.write(reinterpret_cast<const char*>(&pixel), sizeof(pixel));
        return static_cast<bool>(output);
    }
private:
    std::vector<Color> pixels_;
};

}  // namespace

int main(int argc, char* argv[]) {
    const std::string output = argc > 1 ? argv[1] : "screenshots/bresenham_line.ppm";
    const std::vector<Point> firstLine = bresenhamLine(90, 470, 710, 120);
    const std::vector<Point> secondLine = bresenhamLine(100, 140, 690, 450);
    Raster image;
    for (const Point& pixel : firstLine) image.block(pixel.x, pixel.y, 1, {42, 225, 255});
    for (const Point& pixel : secondLine) image.block(pixel.x, pixel.y, 1, {181, 115, 255});
    for (const Point& endpoint : {firstLine.front(), firstLine.back(), secondLine.front(), secondLine.back()})
        image.block(endpoint.x, endpoint.y, 4, {255, 151, 55});
    if (!image.write(output)) {
        std::cerr << "Could not write output image: " << output << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "Bresenham generated " << firstLine.size() + secondLine.size() << " pixel entries.\n";
    std::cout << "Rendered output: " << output << '\n';
}
