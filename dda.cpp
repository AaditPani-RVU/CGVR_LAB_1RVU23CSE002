#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

struct Point {
    int x;
    int y;
};

// Digital Differential Analyzer (DDA) line drawing algorithm
std::vector<Point> ddaLine(int x0, int y0, int x1, int y1) {
    std::vector<Point> points;
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));

    if (steps == 0) {
        return {{x0, y0}};
    }

    const float xIncrement = static_cast<float>(dx) / static_cast<float>(steps);
    const float yIncrement = static_cast<float>(dy) / static_cast<float>(steps);
    float x = static_cast<float>(x0);
    float y = static_cast<float>(y0);

    for (int i = 0; i <= steps; ++i) {
        points.push_back({static_cast<int>(std::round(x)), static_cast<int>(std::round(y))});
        x += xIncrement;
        y += yIncrement;
    }
    return points;
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT, "Lab01 - DDA Line Drawing", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }

    int framebufferWidth;
    int framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    // Two inward-slanting DDA examples, separated from one another.
    std::vector<Point> pixels = ddaLine(-340, 200, -60, -80);
    const GLsizei firstLineCount = static_cast<GLsizei>(pixels.size());
    const std::vector<Point> secondLine = ddaLine(340, 200, 60, -80);
    pixels.insert(pixels.end(), secondLine.begin(), secondLine.end());

    std::vector<float> vertices;
    vertices.reserve(pixels.size() * 2);
    for (const Point& point : pixels) {
        vertices.push_back(static_cast<float>(point.x) / (WINDOW_WIDTH / 2.0f));
        vertices.push_back(static_cast<float>(point.y) / (WINDOW_HEIGHT / 2.0f));
    }

    const char* vertexShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            gl_PointSize = 2.0;
        }
    )";
    const char* fragmentShaderSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.1, 0.8, 1.0, 1.0);
        }
    )";

    const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);
    const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    const GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint vao;
    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glEnable(GL_PROGRAM_POINT_SIZE);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, firstLineCount);
        glDrawArrays(GL_POINTS, firstLineCount, static_cast<GLsizei>(pixels.size()) - firstLineCount);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
