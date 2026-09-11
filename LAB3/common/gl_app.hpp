#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "transform2d.hpp"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// A single RGB-colored vertex, in pixel-space (origin at window center).
struct ColorVertex {
    float x, y;
    float r, g, b;
};

inline void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

inline GLFWwindow* createWindow(const char* title) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        std::exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        std::exit(-1);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD\n";
        std::exit(-1);
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    return window;
}

inline GLuint compileShaderProgram() {
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec3 aColor;
        out vec3 vColor;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            vColor = aColor;
        }
    )";

    const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// Converts pixel-space points (origin at window center) to NDC and appends
// them, colored, as vertices to `out`.
inline void addColoredPolyline(std::vector<ColorVertex>& out, const std::vector<Point2>& pts,
                                float r, float g, float b) {
    for (const Point2& p : pts) {
        float ndcX = p.x / (WINDOW_WIDTH / 2.0f);
        float ndcY = p.y / (WINDOW_HEIGHT / 2.0f);
        out.push_back({ndcX, ndcY, r, g, b});
    }
}

// A drawable run of vertices within the shared VBO: [firstVertex, count) with
// a GL primitive mode (GL_LINE_LOOP for closed shapes, GL_LINES for axes).
struct DrawRange {
    GLenum mode;
    GLint first;
    GLsizei count;
};

inline GLuint uploadVertices(const std::vector<ColorVertex>& vertices, GLuint& vao, GLuint& vbo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ColorVertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return vao;
}

// Saves the current back buffer (before swapping) as a binary PPM (P6) image.
// PPM needs no external dependency; screenshots are converted to PNG afterward.
inline void saveScreenshotPPM(const char* filename, int width, int height) {
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    FILE* f = std::fopen(filename, "wb");
    if (!f) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return;
    }
    std::fprintf(f, "P6\n%d %d\n255\n", width, height);
    // OpenGL reads bottom-up; flip rows so the PPM is top-down.
    for (int row = height - 1; row >= 0; --row) {
        std::fwrite(pixels.data() + static_cast<size_t>(row) * width * 3, 1, static_cast<size_t>(width) * 3, f);
    }
    std::fclose(f);
    std::cout << "Saved screenshot to " << filename << "\n";
}

// Draws the given ranges once and swaps buffers.
inline void renderFrame(GLFWwindow* window, GLuint shaderProgram, GLuint vao,
                         const std::vector<DrawRange>& ranges) {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    for (const DrawRange& range : ranges) {
        glDrawArrays(range.mode, range.first, range.count);
    }
}

// Runs the standard interactive loop: renders each frame, captures a
// screenshot of the very first frame automatically, then keeps the window
// open (Esc or close button to exit) so the result can also be inspected by hand.
inline void runAppLoop(GLFWwindow* window, GLuint shaderProgram, GLuint vao,
                        const std::vector<DrawRange>& ranges, const char* screenshotPath) {
    bool captured = false;
    int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        renderFrame(window, shaderProgram, vao, ranges);

        // Capture a few frames in so the window/driver has fully settled.
        if (!captured && frame == 3) {
            // On HiDPI/Retina displays the framebuffer is larger than the
            // logical window size, so query it rather than assuming it
            // matches WINDOW_WIDTH/WINDOW_HEIGHT.
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            saveScreenshotPPM(screenshotPath, fbWidth, fbHeight);
            captured = true;
        }
        ++frame;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
