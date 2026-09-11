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

struct Vertex {
    float x;
    float y;
    float red;
    float green;
    float blue;
};

struct Color { unsigned char red; unsigned char green; unsigned char blue; };

float edge(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

bool writePreview(const std::string& fileName) {
    const Vertex vertices[] = {
        {-0.75F, -0.65F, 1.0F, 0.15F, 0.20F},
        { 0.75F, -0.65F, 0.20F, 1.0F, 0.35F},
        { 0.00F,  0.75F, 0.20F, 0.45F, 1.0F},
    };
    float sx[3]{};
    float sy[3]{};
    for (int i = 0; i < 3; ++i) {
        sx[i] = (vertices[i].x + 1.0F) * 0.5F * (kWidth - 1);
        sy[i] = (1.0F - vertices[i].y) * 0.5F * (kHeight - 1);
    }

    const float area = edge(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]);
    std::vector<Color> pixels(static_cast<std::size_t>(kWidth) * kHeight, {6, 13, 28});
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            const float px = x + 0.5F;
            const float py = y + 0.5F;
            const float a = edge(sx[1], sy[1], sx[2], sy[2], px, py) / area;
            const float b = edge(sx[2], sy[2], sx[0], sy[0], px, py) / area;
            const float c = edge(sx[0], sy[0], sx[1], sy[1], px, py) / area;
            if (a >= 0.0F && b >= 0.0F && c >= 0.0F) {
                pixels[static_cast<std::size_t>(y) * kWidth + x] = {
                    static_cast<unsigned char>(std::clamp(a * vertices[0].red + b * vertices[1].red + c * vertices[2].red, 0.0F, 1.0F) * 255),
                    static_cast<unsigned char>(std::clamp(a * vertices[0].green + b * vertices[1].green + c * vertices[2].green, 0.0F, 1.0F) * 255),
                    static_cast<unsigned char>(std::clamp(a * vertices[0].blue + b * vertices[1].blue + c * vertices[2].blue, 0.0F, 1.0F) * 255),
                };
            }
        }
    }

    std::ofstream output(fileName, std::ios::binary);
    if (!output) return false;
    output << "P6\n" << kWidth << ' ' << kHeight << "\n255\n";
    for (const Color& pixel : pixels)
        output.write(reinterpret_cast<const char*>(&pixel), sizeof(pixel));
    return static_cast<bool>(output);
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string output = argc > 1 ? argv[1] : "screenshots/vertex_fragment_shader.ppm";
    if (!writePreview(output)) {
        std::cerr << "Could not write output image: " << output << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "Generated the vertex/fragment-shader reference render: " << output << '\n';
    return EXIT_SUCCESS;
}
