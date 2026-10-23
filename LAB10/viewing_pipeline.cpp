// Lab-10: 3D viewing pipeline - projection and viewport mapping.
//
// Every stage of the pipeline is carried out explicitly on the CPU with 4x4
// homogeneous matrices (row-major, column vectors: p' = M * p):
//
//   object --M--> world --V--> eye --P--> clip --/w--> NDC --viewport--> screen
//
// M places each wireframe object in the world, V is a look-at camera, P is
// either a perspective or an orthographic projection, the perspective divide
// gives normalized device coordinates in [-1, 1], and the viewport transform
// maps NDC onto a rectangle of window pixels. Edges are clipped against the
// near plane in eye space before projection.
//
// The same scene is shown through two viewports: perspective on the left,
// orthographic on the right. OpenGL only draws the final 2D pixel-space line
// segments; it never sees a 3D coordinate. The camera orbits the scene, and
// the trace of one vertex through every stage is written to
// screenshots/pipeline_trace.txt.
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

const float PI = 3.14159265f;

const float NEAR_PLANE = 0.5f;
const float FAR_PLANE = 50.0f;

struct Vec3 {
    float x, y, z;
};

struct Vec4 {
    float x, y, z, w;
};

Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 normalize(Vec3 v) {
    float len = std::sqrt(dot(v, v));
    return {v.x / len, v.y / len, v.z / len};
}

struct Mat4 {
    float m[4][4];
};

Mat4 identity() {
    Mat4 r{};
    for (int i = 0; i < 4; ++i) r.m[i][i] = 1.0f;
    return r;
}

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                r.m[row][col] += a.m[row][k] * b.m[k][col];
    return r;
}

Vec4 transform(const Mat4& a, Vec4 p) {
    const float in[4] = {p.x, p.y, p.z, p.w};
    float out[4] = {};
    for (int row = 0; row < 4; ++row)
        for (int k = 0; k < 4; ++k)
            out[row] += a.m[row][k] * in[k];
    return {out[0], out[1], out[2], out[3]};
}

Mat4 translate(float tx, float ty, float tz) {
    Mat4 r = identity();
    r.m[0][3] = tx;
    r.m[1][3] = ty;
    r.m[2][3] = tz;
    return r;
}

Mat4 scale(float sx, float sy, float sz) {
    Mat4 r = identity();
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    r.m[2][2] = sz;
    return r;
}

Mat4 rotateY(float a) {
    Mat4 r = identity();
    r.m[0][0] = std::cos(a);
    r.m[0][2] = std::sin(a);
    r.m[2][0] = -std::sin(a);
    r.m[2][2] = std::cos(a);
    return r;
}

// Viewing transformation: rotate the world so the camera's right (u), up (v)
// and backward (n) axes line up with x, y and z, after moving the eye to
// the origin.
Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 n = normalize(eye - target);
    Vec3 u = normalize(cross(up, n));
    Vec3 v = cross(n, u);
    Mat4 r = identity();
    r.m[0][0] = u.x; r.m[0][1] = u.y; r.m[0][2] = u.z; r.m[0][3] = -dot(u, eye);
    r.m[1][0] = v.x; r.m[1][1] = v.y; r.m[1][2] = v.z; r.m[1][3] = -dot(v, eye);
    r.m[2][0] = n.x; r.m[2][1] = n.y; r.m[2][2] = n.z; r.m[2][3] = -dot(n, eye);
    return r;
}

// Symmetric perspective projection onto the near plane. Sets w = -z_eye so
// the later divide by w shrinks distant points.
Mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
    float f = 1.0f / std::tan(fovY / 2.0f);
    Mat4 r{};
    r.m[0][0] = f / aspect;
    r.m[1][1] = f;
    r.m[2][2] = (zFar + zNear) / (zNear - zFar);
    r.m[2][3] = 2.0f * zFar * zNear / (zNear - zFar);
    r.m[3][2] = -1.0f;
    return r;
}

// Parallel projection: maps the box [l, r] x [b, t] x [-n, -f] to the NDC
// cube; w stays 1, so size does not depend on distance.
Mat4 orthographic(float l, float r, float b, float t, float zNear, float zFar) {
    Mat4 o = identity();
    o.m[0][0] = 2.0f / (r - l);
    o.m[1][1] = 2.0f / (t - b);
    o.m[2][2] = -2.0f / (zFar - zNear);
    o.m[0][3] = -(r + l) / (r - l);
    o.m[1][3] = -(t + b) / (t - b);
    o.m[2][3] = -(zFar + zNear) / (zFar - zNear);
    return o;
}

// A pixel rectangle on the window (origin bottom-left).
struct Viewport {
    float x, y, width, height;
};

// Viewport mapping: NDC [-1, 1] -> window pixels inside the viewport.
Vec3 ndcToScreen(Vec3 ndc, const Viewport& vp) {
    return {vp.x + (ndc.x + 1.0f) * 0.5f * vp.width,
            vp.y + (ndc.y + 1.0f) * 0.5f * vp.height,
            (ndc.z + 1.0f) * 0.5f};
}

struct Edge {
    Vec3 a, b;
    Vec3 color;
};

struct Object {
    std::vector<Vec3> vertices;
    std::vector<std::pair<int, int>> edges;
    Mat4 model;
    Vec3 color;
};

Object makeCube(const Mat4& model, Vec3 color) {
    Object o;
    o.vertices = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                  {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
    o.edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
               {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    o.model = model;
    o.color = color;
    return o;
}

Object makePyramid(const Mat4& model, Vec3 color) {
    Object o;
    o.vertices = {{-1, 0, -1}, {1, 0, -1}, {1, 0, 1}, {-1, 0, 1}, {0, 1.8f, 0}};
    o.edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}};
    o.model = model;
    o.color = color;
    return o;
}

Object makeGround(int halfSize, Vec3 color) {
    Object o;
    for (int i = -halfSize; i <= halfSize; ++i) {
        int base = static_cast<int>(o.vertices.size());
        o.vertices.push_back({static_cast<float>(i), 0, static_cast<float>(-halfSize)});
        o.vertices.push_back({static_cast<float>(i), 0, static_cast<float>(halfSize)});
        o.vertices.push_back({static_cast<float>(-halfSize), 0, static_cast<float>(i)});
        o.vertices.push_back({static_cast<float>(halfSize), 0, static_cast<float>(i)});
        o.edges.push_back({base, base + 1});
        o.edges.push_back({base + 2, base + 3});
    }
    o.model = identity();
    o.color = color;
    return o;
}

std::vector<Object> buildScene() {
    std::vector<Object> scene;
    scene.push_back(makeGround(4, {0.25f, 0.27f, 0.32f}));
    // A row of identical cubes receding along -z shows perspective
    // foreshortening (and its absence in the orthographic view).
    for (int i = 0; i < 4; ++i) {
        Mat4 model = multiply(translate(-1.8f, 0.6f, 2.2f - 2.4f * i), scale(0.6f, 0.6f, 0.6f));
        scene.push_back(makeCube(model, {0.25f, 0.85f, 0.95f}));
    }
    scene.push_back(makePyramid(multiply(translate(1.6f, 0.0f, 0.5f), rotateY(0.4f)), {1.0f, 0.62f, 0.2f}));
    scene.push_back(makeCube(multiply(translate(1.8f, 0.5f, -2.8f), multiply(rotateY(0.7f), scale(0.5f, 0.5f, 0.5f))),
                             {0.6f, 0.95f, 0.45f}));
    return scene;
}

// World-space edges after the modeling transformation.
std::vector<Edge> toWorld(const std::vector<Object>& scene) {
    std::vector<Edge> edges;
    for (const Object& o : scene) {
        for (auto [i, j] : o.edges) {
            Vec4 a = transform(o.model, {o.vertices[i].x, o.vertices[i].y, o.vertices[i].z, 1.0f});
            Vec4 b = transform(o.model, {o.vertices[j].x, o.vertices[j].y, o.vertices[j].z, 1.0f});
            edges.push_back({{a.x, a.y, a.z}, {b.x, b.y, b.z}, o.color});
        }
    }
    return edges;
}

struct ColorVertex {
    float x, y;
    float r, g, b;
};

// Runs world-space edges through view -> projection -> clip -> NDC ->
// viewport and appends the resulting pixel-space segments.
void runPipeline(const std::vector<Edge>& worldEdges, const Mat4& view, const Mat4& projection,
                 const Viewport& vp, std::vector<ColorVertex>& out) {
    for (const Edge& e : worldEdges) {
        Vec4 a = transform(view, {e.a.x, e.a.y, e.a.z, 1.0f});
        Vec4 b = transform(view, {e.b.x, e.b.y, e.b.z, 1.0f});

        // Near-plane clipping in eye space (the camera looks down -z).
        float da = -a.z - NEAR_PLANE;
        float db = -b.z - NEAR_PLANE;
        if (da < 0.0f && db < 0.0f) continue;
        if (da < 0.0f || db < 0.0f) {
            float t = da / (da - db);
            Vec4 hit = {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z), 1.0f};
            (da < 0.0f ? a : b) = hit;
        }

        Vec4 clipA = transform(projection, a);
        Vec4 clipB = transform(projection, b);
        Vec3 ndcA = {clipA.x / clipA.w, clipA.y / clipA.w, clipA.z / clipA.w};
        Vec3 ndcB = {clipB.x / clipB.w, clipB.y / clipB.w, clipB.z / clipB.w};
        Vec3 sa = ndcToScreen(ndcA, vp);
        Vec3 sb = ndcToScreen(ndcB, vp);

        out.push_back({sa.x, sa.y, e.color.x, e.color.y, e.color.z});
        out.push_back({sb.x, sb.y, e.color.x, e.color.y, e.color.z});
    }
}

void addViewportBorder(const Viewport& vp, std::vector<ColorVertex>& out) {
    const float c = 0.55f;
    float x0 = vp.x, y0 = vp.y, x1 = vp.x + vp.width, y1 = vp.y + vp.height;
    const float corners[5][2] = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}};
    for (int i = 0; i < 4; ++i) {
        out.push_back({corners[i][0], corners[i][1], c, c, c});
        out.push_back({corners[i + 1][0], corners[i + 1][1], c, c, c});
    }
}

void printMatrix(FILE* f, const char* name, const Mat4& a) {
    std::fprintf(f, "%s:\n", name);
    for (int row = 0; row < 4; ++row) {
        std::fprintf(f, "[ %9.4f %9.4f %9.4f %9.4f ]\n", a.m[row][0], a.m[row][1], a.m[row][2], a.m[row][3]);
    }
    std::fprintf(f, "\n");
}

// Follows one cube corner through every stage of both pipelines.
void writePipelineTrace(const char* filename, const Object& object, const Mat4& view,
                        const Mat4& perspectiveProj, const Mat4& orthoProj,
                        const Viewport& perspectiveVp, const Viewport& orthoVp, Vec3 eye) {
    FILE* f = std::fopen(filename, "w");
    if (!f) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return;
    }
    std::fprintf(f, "3D viewing pipeline: object -> world -> eye -> clip -> NDC -> screen\n");
    std::fprintf(f, "Camera eye (%.4f, %.4f, %.4f), target (0, 0.5, 0), up (0, 1, 0)\n\n", eye.x, eye.y, eye.z);
    printMatrix(f, "Model matrix M (front cyan cube)", object.model);
    printMatrix(f, "View matrix V (lookAt)", view);
    printMatrix(f, "Perspective projection P (fov 45 deg, aspect 1, near 0.5, far 50)", perspectiveProj);
    printMatrix(f, "Orthographic projection P (l -4.5, r 4.5, b -4.5, t 4.5, near 0.5, far 50)", orthoProj);

    Vec3 v = object.vertices[6];
    Vec4 obj = {v.x, v.y, v.z, 1.0f};
    Vec4 world = transform(object.model, obj);
    Vec4 eyeP = transform(view, world);
    std::fprintf(f, "Vertex traced: cube corner (1, 1, 1)\n");
    std::fprintf(f, "  object : (%9.4f, %9.4f, %9.4f, %9.4f)\n", obj.x, obj.y, obj.z, obj.w);
    std::fprintf(f, "  world  : (%9.4f, %9.4f, %9.4f, %9.4f)\n", world.x, world.y, world.z, world.w);
    std::fprintf(f, "  eye    : (%9.4f, %9.4f, %9.4f, %9.4f)\n", eyeP.x, eyeP.y, eyeP.z, eyeP.w);

    const struct {
        const char* name;
        const Mat4* proj;
        const Viewport* vp;
    } pipelines[2] = {{"Perspective", &perspectiveProj, &perspectiveVp}, {"Orthographic", &orthoProj, &orthoVp}};
    for (const auto& p : pipelines) {
        Vec4 clip = transform(*p.proj, eyeP);
        Vec3 ndc = {clip.x / clip.w, clip.y / clip.w, clip.z / clip.w};
        Vec3 screen = ndcToScreen(ndc, *p.vp);
        std::fprintf(f, "\n  %s (viewport x %.0f, y %.0f, %.0f x %.0f)\n", p.name, p.vp->x, p.vp->y, p.vp->width, p.vp->height);
        std::fprintf(f, "    clip   : (%9.4f, %9.4f, %9.4f, %9.4f)\n", clip.x, clip.y, clip.z, clip.w);
        std::fprintf(f, "    NDC    : (%9.4f, %9.4f, %9.4f)\n", ndc.x, ndc.y, ndc.z);
        std::fprintf(f, "    screen : (%9.4f, %9.4f)  depth %.4f\n", screen.x, screen.y, screen.z);
    }
    std::fclose(f);
    std::cout << "Saved pipeline trace to " << filename << "\n";
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// Converts the window-pixel coordinates produced by the viewport mapping
// back to OpenGL's [-1, 1] range purely so they can be drawn.
GLuint compileShaderProgram() {
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec3 aColor;
        uniform vec2 uWindowSize;
        out vec3 vColor;
        void main() {
            gl_Position = vec4(aPos / uWindowSize * 2.0 - 1.0, 0.0, 1.0);
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

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lab10 - 3D Viewing Pipeline", nullptr, nullptr);
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
    glUseProgram(shaderProgram);
    glUniform2f(glGetUniformLocation(shaderProgram, "uWindowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);

    std::vector<Object> scene = buildScene();
    std::vector<Edge> worldEdges = toWorld(scene);

    // Two square viewports side by side, in window pixels.
    const Viewport perspectiveVp = {20.0f, 20.0f, 460.0f, 460.0f};
    const Viewport orthoVp = {520.0f, 20.0f, 460.0f, 460.0f};
    const Mat4 perspectiveProj = perspective(45.0f * PI / 180.0f, perspectiveVp.width / perspectiveVp.height, NEAR_PLANE, FAR_PLANE);
    const Mat4 orthoProj = orthographic(-4.5f, 4.5f, -4.5f, 4.5f, NEAR_PLANE, FAR_PLANE);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::vector<ColorVertex> vertices;
    int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // The camera orbits the scene; it holds still for the first frames so
        // the screenshot and trace are reproducible.
        float orbit = 0.55f + std::max(0, frame - 10) * 0.005f;
        Vec3 eye = {9.5f * std::sin(orbit), 5.0f, 9.5f * std::cos(orbit)};
        Mat4 view = lookAt(eye, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});

        vertices.clear();
        runPipeline(worldEdges, view, perspectiveProj, perspectiveVp, vertices);
        size_t perspectiveCount = vertices.size();
        runPipeline(worldEdges, view, orthoProj, orthoVp, vertices);
        size_t orthoCount = vertices.size() - perspectiveCount;
        size_t bordersFirst = vertices.size();
        addViewportBorder(perspectiveVp, vertices);
        addViewportBorder(orthoVp, vertices);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ColorVertex), vertices.data(), GL_DYNAMIC_DRAW);

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);

        // Each view is scissored to its own viewport rectangle, so geometry
        // projected outside the NDC square is cut off at the border.
        float pixelScale = static_cast<float>(fbWidth) / WINDOW_WIDTH;
        glEnable(GL_SCISSOR_TEST);
        const struct {
            const Viewport* vp;
            size_t first, count;
        } views[2] = {{&perspectiveVp, 0, perspectiveCount}, {&orthoVp, perspectiveCount, orthoCount}};
        for (const auto& v : views) {
            glScissor(static_cast<GLint>(v.vp->x * pixelScale), static_cast<GLint>(v.vp->y * pixelScale),
                      static_cast<GLsizei>(v.vp->width * pixelScale), static_cast<GLsizei>(v.vp->height * pixelScale));
            glDrawArrays(GL_LINES, static_cast<GLint>(v.first), static_cast<GLsizei>(v.count));
        }
        glDisable(GL_SCISSOR_TEST);
        glDrawArrays(GL_LINES, static_cast<GLint>(bordersFirst), static_cast<GLsizei>(vertices.size() - bordersFirst));

        if (frame == 3) {
            saveScreenshotPPM("screenshots/viewing_pipeline.ppm", fbWidth, fbHeight);
            writePipelineTrace("screenshots/pipeline_trace.txt", scene[1], view, perspectiveProj, orthoProj,
                               perspectiveVp, orthoVp, eye);
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
