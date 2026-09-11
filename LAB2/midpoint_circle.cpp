#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kWidth = 800;
constexpr int kHeight = 600;

struct Point {
    int x;
    int y;
};

struct Color {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

// Midpoint circle algorithm: choose E or SE from p = 1 - r, then reflect each
// chosen point to the other seven octants using eight-way symmetry.
std::vector<Point> midpointCircle(int centerX, int centerY, int radius) {
    std::vector<Point> pixels;
    int x = radius;
    int y = 0;
    int decision = 1 - radius;

    const auto plotEightWaySymmetry = [&](int offsetX, int offsetY) {
        const std::array<Point, 8> symmetricPixels{{
            {centerX + offsetX, centerY + offsetY},
            {centerX + offsetY, centerY + offsetX},
            {centerX - offsetY, centerY + offsetX},
            {centerX - offsetX, centerY + offsetY},
            {centerX - offsetX, centerY - offsetY},
            {centerX - offsetY, centerY - offsetX},
            {centerX + offsetY, centerY - offsetX},
            {centerX + offsetX, centerY - offsetY},
        }};
        pixels.insert(pixels.end(), symmetricPixels.begin(), symmetricPixels.end());
    };

    while (x >= y) {
        plotEightWaySymmetry(x, y);
        ++y;
        if (decision < 0) {
            decision += 2 * y + 1;
        } else {
            --x;
            decision += 2 * (y - x) + 1;
        }
    }
    return pixels;
}

class Raster {
public:
    Raster(int width, int height, Color background)
        : width_(width), height_(height), pixels_(static_cast<std::size_t>(width) * height, background) {}

    void setPixel(int x, int y, Color color) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            pixels_[static_cast<std::size_t>(y) * width_ + x] = color;
        }
    }

    void drawBlock(int centerX, int centerY, int halfSize, Color color) {
        for (int y = centerY - halfSize; y <= centerY + halfSize; ++y) {
            for (int x = centerX - halfSize; x <= centerX + halfSize; ++x) {
                setPixel(x, y, color);
            }
        }
    }

    void drawLine(int x0, int y0, int x1, int y1, Color color) {
        const int dx = std::abs(x1 - x0);
        const int sx = x0 < x1 ? 1 : -1;
        const int dy = -std::abs(y1 - y0);
        const int sy = y0 < y1 ? 1 : -1;
        int error = dx + dy;
        while (true) {
            setPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) {
                return;
            }
            const int twiceError = 2 * error;
            if (twiceError >= dy) {
                error += dy;
                x0 += sx;
            }
            if (twiceError <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    }

    bool writePpm(const std::string& path) const {
        std::ofstream output(path, std::ios::binary);
        if (!output) {
            return false;
        }
        output << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        for (const Color& pixel : pixels_) {
            output.write(reinterpret_cast<const char*>(&pixel), sizeof(pixel));
        }
        return static_cast<bool>(output);
    }

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;
};

}  // namespace

int main(int argc, char* argv[]) {
    const std::string outputFile = argc > 1 ? argv[1] : "screenshots/midpoint_circle.ppm";
    constexpr Color background{6, 12, 28};
    constexpr Color axis{42, 70, 100};
    constexpr Color circle{42, 225, 255};
    constexpr Color center{255, 151, 55};

    Raster image(kWidth, kHeight, background);
    const int centerX = kWidth / 2;
    const int centerY = kHeight / 2;
    image.drawLine(0, centerY, kWidth - 1, centerY, axis);
    image.drawLine(centerX, 0, centerX, kHeight - 1, axis);

    const std::vector<Point> pixels = midpointCircle(centerX, centerY, 200);
    for (const Point& pixel : pixels) {
        image.drawBlock(pixel.x, pixel.y, 1, circle);
    }
    image.drawBlock(centerX, centerY, 4, center);

    if (!image.writePpm(outputFile)) {
        std::cerr << "Could not write output image: " << outputFile << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "Midpoint circle generated " << pixels.size()
              << " symmetric pixel entries (radius = 200).\n";
    std::cout << "Rendered output: " << outputFile << '\n';
    return EXIT_SUCCESS;
}
