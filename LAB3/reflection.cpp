// Lab-3: 2D reflection using homogeneous coordinates.
//
// Reflection about the X axis:      Reflection about the Y axis:
//   [1  0  0]   [x]                   [-1 0  0]   [x]
//   [0 -1  0] * [y]                   [ 0 1  0] * [y]
//   [0  0  1]   [1]                   [ 0 0  1]   [1]
#include "common/gl_app.hpp"
#include "common/scene.hpp"
#include "common/transform2d.hpp"

int main() {
    GLFWwindow* window = createWindow("Lab3 - Reflection");
    GLuint shaderProgram = compileShaderProgram();

    std::vector<Point2> original = sampleShape();
    std::vector<Point2> reflectedX = applyTransform(reflectionMatrixX(), original);
    std::vector<Point2> reflectedY = applyTransform(reflectionMatrixY(), original);

    std::vector<ColorVertex> vertices;
    std::vector<DrawRange> ranges;

    std::vector<Point2> axes = axisLines();
    addColoredPolyline(vertices, axes, 0.4f, 0.4f, 0.4f);
    ranges.push_back({GL_LINES, 0, static_cast<GLsizei>(axes.size())});

    GLint start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, original, 1.0f, 1.0f, 1.0f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(original.size())});

    start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, reflectedX, 1.0f, 0.3f, 0.3f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(reflectedX.size())});

    start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, reflectedY, 1.0f, 0.9f, 0.2f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(reflectedY.size())});

    GLuint vao, vbo;
    uploadVertices(vertices, vao, vbo);

    runAppLoop(window, shaderProgram, vao, ranges, "screenshots/reflection.ppm");

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
