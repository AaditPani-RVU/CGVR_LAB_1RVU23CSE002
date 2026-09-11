#include <algorithm>
#include <cmath>
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

std::vector<Point> ddaLine(int x0, int y0, int x1, int y1) {
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    std::vector<Point> pixels;
    if (steps == 0) return {{x0, y0}};

    const float xStep = static_cast<float>(dx) / steps;
    const float yStep = static_cast<float>(dy) / steps;
    float x = static_cast<float>(x0);
    float y = static_cast<float>(y0);
    for (int step = 0; step <= steps; ++step) {
        pixels.push_back({static_cast<int>(std::round(x)), static_cast<int>(std::round(y))});
        x += xStep;
        y += yStep;
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
    const std::string output = argc > 1 ? argv[1] : "screenshots/dda_line.ppm";
    const std::vector<Point> firstLine = ddaLine(90, 470, 710, 120);
    const std::vector<Point> secondLine = ddaLine(100, 140, 690, 450);
    Raster image;
    for (const Point& pixel : firstLine) image.block(pixel.x, pixel.y, 1, {42, 225, 255});
    for (const Point& pixel : secondLine) image.block(pixel.x, pixel.y, 1, {181, 115, 255});
    for (const Point& endpoint : {firstLine.front(), firstLine.back(), secondLine.front(), secondLine.back()})
        image.block(endpoint.x, endpoint.y, 4, {255, 151, 55});
    if (!image.write(output)) {
        std::cerr << "Could not write output image: " << output << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "DDA generated " << firstLine.size() + secondLine.size() << " pixel entries.\n";
    std::cout << "Rendered output: " << output << '\n';
}
