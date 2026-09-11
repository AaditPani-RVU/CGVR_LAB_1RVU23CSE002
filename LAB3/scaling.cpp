// Lab-3: 2D scaling about the origin using homogeneous coordinates.
//
//   [sx  0  0]   [x]
//   [ 0 sy  0] * [y]
//   [ 0  0  1]   [1]
#include "common/gl_app.hpp"
#include "common/scene.hpp"
#include "common/transform2d.hpp"

int main() {
    GLFWwindow* window = createWindow("Lab3 - Scaling");
    GLuint shaderProgram = compileShaderProgram();

    std::vector<Point2> original = sampleShape();
    Mat3 scaling = scalingMatrix(1.6f, 0.6f);
    std::vector<Point2> scaled = applyTransform(scaling, original);

    std::vector<ColorVertex> vertices;
    std::vector<DrawRange> ranges;

    std::vector<Point2> axes = axisLines();
    addColoredPolyline(vertices, axes, 0.4f, 0.4f, 0.4f);
    ranges.push_back({GL_LINES, 0, static_cast<GLsizei>(axes.size())});

    GLint start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, original, 1.0f, 1.0f, 1.0f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(original.size())});

    start = static_cast<GLint>(vertices.size());
    addColoredPolyline(vertices, scaled, 0.6f, 1.0f, 0.4f);
    ranges.push_back({GL_LINE_LOOP, start, static_cast<GLsizei>(scaled.size())});

    GLuint vao, vbo;
    uploadVertices(vertices, vao, vbo);

    runAppLoop(window, shaderProgram, vao, ranges, "screenshots/scaling.ppm");

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
