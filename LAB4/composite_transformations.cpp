#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kWidth = 800;
constexpr int kHeight = 600;
constexpr float kPi = 3.14159265358979323846F;

struct Point2 { float x; float y; };
struct Pixel { int x; int y; };
struct Color { unsigned char red; unsigned char green; unsigned char blue; };

// Row-major homogeneous-coordinate matrix using column vectors: p' = M * p.
struct Mat3 {
    float m[3][3];
};

Mat3 multiply(const Mat3& left, const Mat3& right) {
    Mat3 product{};
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            for (int k = 0; k < 3; ++k)
                product.m[row][col] += left.m[row][k] * right.m[k][col];
    return product;
}

Mat3 scale(float sx, float sy) {
    return {{{sx, 0.0F, 0.0F}, {0.0F, sy, 0.0F}, {0.0F, 0.0F, 1.0F}}};
}

Mat3 rotate(float degrees) {
    const float radians = degrees * kPi / 180.0F;
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return {{{c, -s, 0.0F}, {s, c, 0.0F}, {0.0F, 0.0F, 1.0F}}};
}

Mat3 translate(float tx, float ty) {
    return {{{1.0F, 0.0F, tx}, {0.0F, 1.0F, ty}, {0.0F, 0.0F, 1.0F}}};
}

Point2 transform(const Mat3& matrix, const Point2& point) {
    return {
        matrix.m[0][0] * point.x + matrix.m[0][1] * point.y + matrix.m[0][2],
        matrix.m[1][0] * point.x + matrix.m[1][1] * point.y + matrix.m[1][2],
    };
}

std::vector<Point2> transform(const Mat3& matrix, const std::vector<Point2>& shape) {
    std::vector<Point2> transformed;
    transformed.reserve(shape.size());
    for (const Point2& point : shape) transformed.push_back(transform(matrix, point));
    return transformed;
}

Pixel toPixel(const Point2& point) {
    return {static_cast<int>(std::lround(kWidth / 2.0F + point.x)),
            static_cast<int>(std::lround(kHeight / 2.0F - point.y))};
}

class Raster {
public:
    Raster() : pixels_(static_cast<std::size_t>(kWidth) * kHeight, {6, 12, 28}) {}

    void setPixel(int x, int y, Color color) {
        if (x >= 0 && x < kWidth && y >= 0 && y < kHeight)
            pixels_[static_cast<std::size_t>(y) * kWidth + x] = color;
    }

    void drawBlock(int x, int y, int radius, Color color) {
        for (int row = y - radius; row <= y + radius; ++row)
            for (int col = x - radius; col <= x + radius; ++col) setPixel(col, row, color);
    }

    void drawLine(Pixel start, Pixel end, Color color) {
        int x = start.x;
        int y = start.y;
        const int dx = std::abs(end.x - start.x);
        const int sx = x < end.x ? 1 : -1;
        const int dy = -std::abs(end.y - start.y);
        const int sy = y < end.y ? 1 : -1;
        int error = dx + dy;
        while (true) {
            setPixel(x, y, color);
            if (x == end.x && y == end.y) break;
            const int twiceError = 2 * error;
            if (twiceError >= dy) { error += dy; x += sx; }
            if (twiceError <= dx) { error += dx; y += sy; }
        }
    }

    void drawClosedPolygon(const std::vector<Point2>& polygon, Color edge, Color vertex) {
        for (std::size_t i = 0; i < polygon.size(); ++i) {
            const Pixel from = toPixel(polygon[i]);
            const Pixel to = toPixel(polygon[(i + 1) % polygon.size()]);
            drawLine(from, to, edge);
            drawBlock(from.x, from.y, 3, vertex);
        }
    }

    bool writePpm(const std::string& path) const {
        std::ofstream output(path, std::ios::binary);
        if (!output) return false;
        output << "P6\n" << kWidth << ' ' << kHeight << "\n255\n";
        for (const Color& pixel : pixels_)
            output.write(reinterpret_cast<const char*>(&pixel), sizeof(pixel));
        return static_cast<bool>(output);
    }

private:
    std::vector<Color> pixels_;
};

bool writeMatrixReport(const std::string& path, const Mat3& composite,
                       const std::vector<Point2>& original, const std::vector<Point2>& final) {
    std::ofstream output(path);
    if (!output) return false;
    output << std::fixed << std::setprecision(4);
    output << "Composite transformation order: scale -> rotate -> translate\n";
    output << "M = T(220, 90) * R(35 deg) * S(0.75, 0.75)\n\n";
    output << "Composite matrix:\n";
    for (const auto& row : composite.m)
        output << "[ " << std::setw(8) << row[0] << "  " << std::setw(8) << row[1]
               << "  " << std::setw(8) << row[2] << " ]\n";
    output << "\nOriginal point -> transformed point\n";
    for (std::size_t i = 0; i < original.size(); ++i)
        output << "(" << original[i].x << ", " << original[i].y << ") -> ("
               << final[i].x << ", " << final[i].y << ")\n";
    return static_cast<bool>(output);
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string imageOutput = argc > 1 ? argv[1] : "screenshots/composite_transformations.ppm";
    const std::string reportOutput = argc > 2 ? argv[2] : "screenshots/composite_matrix.txt";

    // An asymmetric L-shaped polygon makes rotation and the final displacement clear.
    const std::vector<Point2> original{{-130, -100}, {50, -100}, {50, -35},
                                       {130, -35}, {130, 100}, {-130, 100}};
    const Mat3 composite = multiply(translate(220.0F, 90.0F), multiply(rotate(35.0F), scale(0.75F, 0.75F)));
    const std::vector<Point2> finalShape = transform(composite, original);

    Raster image;
    constexpr Color axis{39, 72, 103};
    image.drawLine({0, kHeight / 2}, {kWidth - 1, kHeight / 2}, axis);
    image.drawLine({kWidth / 2, 0}, {kWidth / 2, kHeight - 1}, axis);
    image.drawClosedPolygon(original, {162, 176, 194}, {235, 242, 255});
    image.drawClosedPolygon(finalShape, {42, 225, 255}, {255, 151, 55});

    if (!image.writePpm(imageOutput) || !writeMatrixReport(reportOutput, composite, original, finalShape)) {
        std::cerr << "Could not write the Lab 4 output files.\n";
        return EXIT_FAILURE;
    }
    std::cout << "Applied M = T(220,90) * R(35 deg) * S(0.75,0.75) to "
              << original.size() << " vertices.\n";
    std::cout << "Rendered output: " << imageOutput << '\n';
    std::cout << "Matrix report: " << reportOutput << '\n';
    return EXIT_SUCCESS;
}
