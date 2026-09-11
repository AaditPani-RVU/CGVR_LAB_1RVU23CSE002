// Lab-3: 2D rotation about the origin using homogeneous coordinates.
//
//   [cos(t) -sin(t) 0]   [x]
//   [sin(t)  cos(t) 0] * [y]
//   [  0       0    1]   [1]
#include "common/gl_app.hpp"
#include "common/scene.hpp"
#include "common/transform2d.hpp"

int main() {
    GLFWwindow* window = createWindow("Lab3 - Rotation");
    GLuint shaderProgram = compileShaderProgram();

    std::vector<Point2> original = sampleShape();
    Mat3 rotation = rotationMatrix(60.0f);
    std::vector<Point2> rotated = applyTransform(rotation, original);

    std::vector<ColorVertex> vertices;
    std::vector<DrawRange> ranges;

    std::vector<Point2> axes = axisLines();
    addColoredPolyline(vertices, axes, 0.4f, 0.4f, 0.4f);
    ranges.push_back({GL_LINES, 0, static_cast<GLsizei>(axes.size())});

    GLint start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, original, 1.0f, 1.0f, 1.0f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(original.size())});

    start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, rotated, 0.3f, 0.8f, 1.0f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(rotated.size())});

    GLuint vao, vbo;
    uploadVertices(vertices, vao, vbo);

    runAppLoop(window, shaderProgram, vao, ranges, "screenshots/rotation.ppm");

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
