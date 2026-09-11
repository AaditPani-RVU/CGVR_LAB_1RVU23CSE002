// Lab-5: Cohen-Sutherland line clipping algorithm.
//
// Each candidate line is classified against the clip window using a 4-bit
// outcode (LEFT | RIGHT | BOTTOM | TOP). Lines with both endpoints sharing a
// bit are trivially rejected; lines with both outcodes zero are trivially
// accepted; everything else is clipped one window edge at a time until one
// of those two cases is reached.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Clip window, in the same pixel-space (origin at window center) as the
// scene geometry below.
const double CLIP_XMIN = -150.0;
const double CLIP_XMAX = 150.0;
const double CLIP_YMIN = -100.0;
const double CLIP_YMAX = 100.0;

enum OutCode { INSIDE = 0, LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8 };

int computeOutCode(double x, double y) {
    int code = INSIDE;
    if (x < CLIP_XMIN) code |= LEFT;
    else if (x > CLIP_XMAX) code |= RIGHT;
    if (y < CLIP_YMIN) code |= BOTTOM;
    else if (y > CLIP_YMAX) code |= TOP;
    return code;
}

// Clips the segment (x0,y0)-(x1,y1) against the clip window in place.
// Returns true if any part of the segment survives.
bool cohenSutherlandClip(double& x0, double& y0, double& x1, double& y1) {
    int outcode0 = computeOutCode(x0, y0);
    int outcode1 = computeOutCode(x1, y1);

    while (true) {
        if ((outcode0 | outcode1) == 0) {
            return true;  // Both endpoints inside: trivial accept.
        }
        if ((outcode0 & outcode1) != 0) {
            return false;  // Both endpoints share an outside region: trivial reject.
        }

        int outcodeOut = outcode0 != 0 ? outcode0 : outcode1;
        double x = 0.0, y = 0.0;

        if (outcodeOut & TOP) {
            x = x0 + (x1 - x0) * (CLIP_YMAX - y0) / (y1 - y0);
            y = CLIP_YMAX;
        } else if (outcodeOut & BOTTOM) {
            x = x0 + (x1 - x0) * (CLIP_YMIN - y0) / (y1 - y0);
            y = CLIP_YMIN;
        } else if (outcodeOut & RIGHT) {
            y = y0 + (y1 - y0) * (CLIP_XMAX - x0) / (x1 - x0);
            x = CLIP_XMAX;
        } else if (outcodeOut & LEFT) {
            y = y0 + (y1 - y0) * (CLIP_XMIN - x0) / (x1 - x0);
            x = CLIP_XMIN;
        }

        if (outcodeOut == outcode0) {
            x0 = x;
            y0 = y;
            outcode0 = computeOutCode(x0, y0);
        } else {
            x1 = x;
            y1 = y;
            outcode1 = computeOutCode(x1, y1);
        }
    }
}

struct ColorVertex {
    float x, y;
    float r, g, b;
};

struct Line {
    double x0, y0, x1, y1;
};

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint compileShaderProgram() {
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

void addVertex(std::vector<ColorVertex>& out, double x, double y, float r, float g, float b) {
    float ndcX = static_cast<float>(x) / (WINDOW_WIDTH / 2.0f);
    float ndcY = static_cast<float>(y) / (WINDOW_HEIGHT / 2.0f);
    out.push_back({ndcX, ndcY, r, g, b});
}

void saveScreenshotPPM(const char* filename, int width, int height) {
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

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lab5 - Cohen-Sutherland Line Clipping", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    GLuint shaderProgram = compileShaderProgram();

    // Candidate lines (pixel-space, origin at window center) covering the
    // classic Cohen-Sutherland cases: fully inside, fully outside (trivial
    // reject), crossing the left/right edges, crossing the top/bottom edges,
    // and a diagonal crossing a corner region.
    std::vector<Line> testLines = {
        {-50, -50, 80, 60},          // fully inside
        {-350, -250, -220, -180},    // fully outside: trivial reject
        {-300, 20, 300, -20},        // crosses left and right edges
        {-20, -250, 30, 250},        // crosses bottom and top edges
        {-250, -180, 200, 150},      // diagonal crossing a corner region
    };

    std::vector<ColorVertex> vertices;

    // Clip window outline, in white.
    std::vector<ColorVertex> windowOutline;
    addVertex(windowOutline, CLIP_XMIN, CLIP_YMIN, 1.0f, 1.0f, 1.0f);
    addVertex(windowOutline, CLIP_XMAX, CLIP_YMIN, 1.0f, 1.0f, 1.0f);
    addVertex(windowOutline, CLIP_XMAX, CLIP_YMAX, 1.0f, 1.0f, 1.0f);
    addVertex(windowOutline, CLIP_XMIN, CLIP_YMAX, 1.0f, 1.0f, 1.0f);
    GLint windowFirst = 0;
    vertices.insert(vertices.end(), windowOutline.begin(), windowOutline.end());
    GLsizei windowCount = static_cast<GLsizei>(windowOutline.size());

    // Original (unclipped) lines, in steel blue, and the accepted clipped
    // segment on top, in bright green. Rejected lines show only their
    // original (dim red) segment - no clipped portion is drawn for them.
    GLint linesFirst = static_cast<GLint>(vertices.size());
    for (const Line& line : testLines) {
        double cx0 = line.x0, cy0 = line.y0, cx1 = line.x1, cy1 = line.y1;
        bool accepted = cohenSutherlandClip(cx0, cy0, cx1, cy1);

        if (accepted) {
            addVertex(vertices, line.x0, line.y0, 0.35f, 0.45f, 0.65f);
            addVertex(vertices, line.x1, line.y1, 0.35f, 0.45f, 0.65f);
        } else {
            addVertex(vertices, line.x0, line.y0, 0.8f, 0.2f, 0.2f);
            addVertex(vertices, line.x1, line.y1, 0.8f, 0.2f, 0.2f);
        }
    }
    GLsizei linesCount = static_cast<GLsizei>(vertices.size() - linesFirst);

    GLint clippedFirst = static_cast<GLint>(vertices.size());
    for (const Line& line : testLines) {
        double cx0 = line.x0, cy0 = line.y0, cx1 = line.x1, cy1 = line.y1;
        if (cohenSutherlandClip(cx0, cy0, cx1, cy1)) {
            addVertex(vertices, cx0, cy0, 0.2f, 1.0f, 0.3f);
            addVertex(vertices, cx1, cy1, 0.2f, 1.0f, 0.3f);
        }
    }
    GLsizei clippedCount = static_cast<GLsizei>(vertices.size() - clippedFirst);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ColorVertex), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    bool captured = false;
    int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        glDrawArrays(GL_LINE_LOOP, windowFirst, windowCount);
        glDrawArrays(GL_LINES, linesFirst, linesCount);
        glDrawArrays(GL_LINES, clippedFirst, clippedCount);

        if (!captured && frame == 3) {
            int currentFbWidth, currentFbHeight;
            glfwGetFramebufferSize(window, &currentFbWidth, &currentFbHeight);
            saveScreenshotPPM("screenshots/cohen_sutherland.ppm", currentFbWidth, currentFbHeight);
            captured = true;
        }
        ++frame;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
