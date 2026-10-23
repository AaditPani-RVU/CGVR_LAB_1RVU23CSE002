// Lab-6: Z-buffer algorithm for hidden surface removal.
//
// The scene (two interpenetrating cubes and a tilted plane slicing through
// them) is scan-converted on the CPU into a color buffer and a depth buffer.
// For every pixel a triangle covers, its interpolated depth is compared with
// the value already stored in the Z-buffer; the fragment is written only if
// it is closer. Because the test is per pixel, intersecting surfaces resolve
// correctly - something no ordering of whole polygons (painter's algorithm)
// can achieve.
//
// OpenGL is used only to show the two software buffers side by side: the
// shaded color buffer on the left and the depth buffer (near = bright) on
// the right. Press Space to toggle the depth test and see the scene drawn in
// plain submission order instead.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;

// Resolution of the software color and depth buffers (one panel each).
const int BUFFER_WIDTH = 500;
const int BUFFER_HEIGHT = 500;

const float PI = 3.14159265f;

struct Vec3 {
    float x, y, z;
};

Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }

Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 normalize(Vec3 v) {
    float len = std::sqrt(dot(v, v));
    return len > 0.0f ? v * (1.0f / len) : v;
}

Vec3 rotateX(Vec3 v, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

Vec3 rotateY(Vec3 v, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

Vec3 rotateZ(Vec3 v, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

struct Triangle {
    Vec3 v[3];  // world space
    Vec3 color;
};

// Software frame: an RGB color buffer and a float depth buffer, both stored
// bottom row first so they upload directly as OpenGL textures.
struct FrameBuffers {
    std::vector<unsigned char> color;
    std::vector<float> depth;
};

void addCube(std::vector<Triangle>& out, Vec3 center, float size, float angleX, float angleY, Vec3 color) {
    float h = size / 2.0f;
    Vec3 corners[8] = {
        {-h, -h, -h}, {h, -h, -h}, {h, h, -h}, {-h, h, -h},
        {-h, -h, h},  {h, -h, h},  {h, h, h},  {-h, h, h},
    };
    for (Vec3& c : corners) {
        c = rotateY(rotateX(c, angleX), angleY) + center;
    }
    const int faces[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0},
    };
    for (const auto& f : faces) {
        out.push_back({{corners[f[0]], corners[f[1]], corners[f[2]]}, color});
        out.push_back({{corners[f[0]], corners[f[2]], corners[f[3]]}, color});
    }
}

void addTiltedPlane(std::vector<Triangle>& out, float halfWidth, float halfDepth, float tilt, float height, Vec3 color) {
    Vec3 p[4] = {
        {-halfWidth, 0.0f, -halfDepth}, {halfWidth, 0.0f, -halfDepth},
        {halfWidth, 0.0f, halfDepth},   {-halfWidth, 0.0f, halfDepth},
    };
    for (Vec3& c : p) {
        c = rotateZ(c, tilt) + Vec3{0.0f, height, 0.0f};
    }
    out.push_back({{p[0], p[1], p[2]}, color});
    out.push_back({{p[0], p[2], p[3]}, color});
}

// Submission order is back cube, front cube, then the plane. Without the
// Z-buffer the plane, drawn last, wrongly covers everything it overlaps.
std::vector<Triangle> buildScene(float spin) {
    std::vector<Triangle> scene;
    addCube(scene, {0.75f, 0.25f, -0.6f}, 1.3f, 0.35f, 0.5f, {0.20f, 0.75f, 0.75f});
    addCube(scene, {-0.55f, -0.05f, 0.1f}, 1.4f, 0.45f, spin, {0.95f, 0.55f, 0.15f});
    addTiltedPlane(scene, 2.0f, 1.6f, 0.30f, 0.05f, {0.55f, 0.55f, 0.85f});
    return scene;
}

// Camera: looks down -Z from z = 6, slightly above the scene, with a
// symmetric perspective projection (fov 45 degrees, near 1, far 12).
const float CAMERA_DISTANCE = 6.0f;
const float CAMERA_PITCH = 0.35f;
const float FOV_Y = 45.0f * PI / 180.0f;
const float NEAR_PLANE = 1.0f;
const float FAR_PLANE = 12.0f;

// World point -> screen x, y (pixels) and z (depth in [0, 1], 0 = near).
Vec3 projectToScreen(Vec3 world) {
    Vec3 view = rotateX(world, CAMERA_PITCH);
    view.z -= CAMERA_DISTANCE;

    float f = 1.0f / std::tan(FOV_Y / 2.0f);
    float aspect = static_cast<float>(BUFFER_WIDTH) / BUFFER_HEIGHT;
    float clipX = f / aspect * view.x;
    float clipY = f * view.y;
    float clipZ = (FAR_PLANE + NEAR_PLANE) / (NEAR_PLANE - FAR_PLANE) * view.z +
                  2.0f * FAR_PLANE * NEAR_PLANE / (NEAR_PLANE - FAR_PLANE);
    float clipW = -view.z;

    // Perspective divide -> NDC -> viewport.
    float ndcX = clipX / clipW, ndcY = clipY / clipW, ndcZ = clipZ / clipW;
    return {(ndcX + 1.0f) * 0.5f * BUFFER_WIDTH, (ndcY + 1.0f) * 0.5f * BUFFER_HEIGHT, (ndcZ + 1.0f) * 0.5f};
}

float edgeFunction(Vec3 a, Vec3 b, float px, float py) {
    return (px - a.x) * (b.y - a.y) - (py - a.y) * (b.x - a.x);
}

void rasterizeScene(const std::vector<Triangle>& scene, bool useZBuffer, FrameBuffers& fb) {
    // Initialise: background color, and every depth to the far plane (1.0).
    fb.color.assign(static_cast<size_t>(BUFFER_WIDTH) * BUFFER_HEIGHT * 3, 0);
    for (size_t i = 0; i < fb.color.size(); i += 3) {
        fb.color[i] = 13;
        fb.color[i + 1] = 15;
        fb.color[i + 2] = 22;
    }
    fb.depth.assign(static_cast<size_t>(BUFFER_WIDTH) * BUFFER_HEIGHT, 1.0f);

    const Vec3 lightDir = normalize({0.4f, 0.8f, 0.6f});

    for (const Triangle& tri : scene) {
        // Flat (Lambert) shading from the face normal; two-sided because
        // nothing is back-face culled - the Z-buffer alone hides surfaces.
        Vec3 normal = normalize(cross(tri.v[1] - tri.v[0], tri.v[2] - tri.v[0]));
        float intensity = 0.3f + 0.7f * std::fabs(dot(normal, lightDir));
        unsigned char r = static_cast<unsigned char>(std::min(1.0f, tri.color.x * intensity) * 255.0f);
        unsigned char g = static_cast<unsigned char>(std::min(1.0f, tri.color.y * intensity) * 255.0f);
        unsigned char b = static_cast<unsigned char>(std::min(1.0f, tri.color.z * intensity) * 255.0f);

        Vec3 s0 = projectToScreen(tri.v[0]);
        Vec3 s1 = projectToScreen(tri.v[1]);
        Vec3 s2 = projectToScreen(tri.v[2]);
        float area = edgeFunction(s0, s1, s2.x, s2.y);
        if (std::fabs(area) < 1e-6f) continue;

        int minX = std::max(0, static_cast<int>(std::floor(std::min({s0.x, s1.x, s2.x}))));
        int maxX = std::min(BUFFER_WIDTH - 1, static_cast<int>(std::ceil(std::max({s0.x, s1.x, s2.x}))));
        int minY = std::max(0, static_cast<int>(std::floor(std::min({s0.y, s1.y, s2.y}))));
        int maxY = std::min(BUFFER_HEIGHT - 1, static_cast<int>(std::ceil(std::max({s0.y, s1.y, s2.y}))));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float px = x + 0.5f, py = y + 0.5f;
                // Barycentric weights; dividing by the signed area makes
                // them non-negative inside regardless of winding.
                float w0 = edgeFunction(s1, s2, px, py) / area;
                float w1 = edgeFunction(s2, s0, px, py) / area;
                float w2 = edgeFunction(s0, s1, px, py) / area;
                if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;

                // Screen-space depth is affine in x and y after the
                // perspective divide, so linear interpolation is exact.
                float z = w0 * s0.z + w1 * s1.z + w2 * s2.z;
                size_t index = static_cast<size_t>(y) * BUFFER_WIDTH + x;

                // The Z-buffer test: keep the fragment only if it is nearer
                // than whatever has been drawn at this pixel so far.
                if (useZBuffer && z >= fb.depth[index]) continue;

                fb.depth[index] = z;
                fb.color[index * 3] = r;
                fb.color[index * 3 + 1] = g;
                fb.color[index * 3 + 2] = b;
            }
        }
    }
}

// Depth buffer as grayscale: nearest written depth -> white, farthest ->
// dark gray, untouched (far plane) pixels -> black.
std::vector<unsigned char> depthToGrayscale(const std::vector<float>& depth) {
    float nearest = 1.0f, farthest = 0.0f;
    for (float d : depth) {
        if (d < 1.0f) {
            nearest = std::min(nearest, d);
            farthest = std::max(farthest, d);
        }
    }
    float range = std::max(farthest - nearest, 1e-6f);

    std::vector<unsigned char> gray(depth.size() * 3, 0);
    for (size_t i = 0; i < depth.size(); ++i) {
        if (depth[i] >= 1.0f) continue;
        float t = 1.0f - (depth[i] - nearest) / range;
        unsigned char v = static_cast<unsigned char>((0.15f + 0.85f * t) * 255.0f);
        gray[i * 3] = gray[i * 3 + 1] = gray[i * 3 + 2] = v;
    }
    return gray;
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint compileShaderProgram() {
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            vTexCoord = aTexCoord;
        }
    )";

    const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform sampler2D uImage;
        void main() {
            FragColor = texture(uImage, vTexCoord);
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

GLuint createTexture() {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, BUFFER_WIDTH, BUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    return texture;
}

void saveScreenshotPPM(const char* filename, int width, int height) {
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
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

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lab6 - Z-Buffer Hidden Surface Removal", nullptr, nullptr);
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

    GLuint shaderProgram = compileShaderProgram();

    // Full-panel quad: position (x, y), then texture coordinate (u, v).
    const float quad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint colorTexture = createTexture();
    GLuint depthTexture = createTexture();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    FrameBuffers fb;
    bool useZBuffer = true;
    bool spaceWasDown = false;
    int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceDown && !spaceWasDown) {
            useZBuffer = !useZBuffer;
        }
        spaceWasDown = spaceDown;

        // Two screenshots are captured automatically: with the Z-buffer
        // (frame 3) and without it (frame 7). The spin is held still until
        // both are taken so they show the same pose.
        if (frame == 5) useZBuffer = false;
        if (frame == 9) useZBuffer = true;
        float spin = 0.6f + std::max(0, frame - 10) * 0.01f;

        rasterizeScene(buildScene(spin), useZBuffer, fb);
        std::vector<unsigned char> depthImage = depthToGrayscale(fb.depth);

        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BUFFER_WIDTH, BUFFER_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, fb.color.data());
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BUFFER_WIDTH, BUFFER_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, depthImage.data());

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);

        // Left panel: color buffer. Right panel: depth buffer.
        glViewport(0, 0, fbWidth / 2, fbHeight);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glViewport(fbWidth / 2, 0, fbWidth - fbWidth / 2, fbHeight);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (frame == 3) {
            saveScreenshotPPM("screenshots/z_buffer.ppm", fbWidth, fbHeight);
        } else if (frame == 7) {
            saveScreenshotPPM("screenshots/without_z_buffer.ppm", fbWidth, fbHeight);
        }
        ++frame;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &colorTexture);
    glDeleteTextures(1, &depthTexture);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
