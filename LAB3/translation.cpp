// Lab-3: 2D translation using homogeneous coordinates.
//
// A point (x, y) is translated by (tx, ty) via the 3x3 homogeneous matrix
//   [1 0 tx]   [x]
//   [0 1 ty] * [y]
//   [0 0  1]   [1]
#include "common/gl_app.hpp"
#include "common/scene.hpp"
#include "common/transform2d.hpp"

int main() {
    GLFWwindow* window = createWindow("Lab3 - Translation");
    GLuint shaderProgram = compileShaderProgram();

    std::vector<Point2> original = sampleShape();
    Mat3 translation = translationMatrix(150.0f, 90.0f);
    std::vector<Point2> translated = applyTransform(translation, original);

    std::vector<ColorVertex> vertices;
    std::vector<DrawRange> ranges;

    std::vector<Point2> axes = axisLines();
    addColoredPolyline(vertices, axes, 0.4f, 0.4f, 0.4f);
    ranges.push_back({GL_LINES, 0, static_cast<GLsizei>(axes.size())});

    GLint start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, original, 1.0f, 1.0f, 1.0f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(original.size())});

    start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, translated, 1.0f, 0.55f, 0.0f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(translated.size())});

    GLuint vao, vbo;
    uploadVertices(vertices, vao, vbo);

    runAppLoop(window, shaderProgram, vao, ranges, "screenshots/translation.ppm");

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
