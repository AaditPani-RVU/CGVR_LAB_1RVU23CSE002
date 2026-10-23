// Lab-7: Phong illumination model for a 3D object.
//
// A UV sphere is lit per fragment with the Phong reflection model:
//
//   I = Ia * ka  +  Il * kd * max(N.L, 0)  +  Il * ks * max(R.V, 0)^n
//
// where N is the surface normal, L the direction to the light, V the
// direction to the viewer and R = reflect(-L, N) the mirror direction of the
// light about the normal. The window is split into four viewports that show
// the ambient, diffuse and specular terms on their own and, bottom right,
// their sum - the full Phong result. The light orbits the sphere so the
// diffuse falloff and the specular highlight can be seen moving.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

const float PI = 3.14159265f;

// Column-major 4x4 matrix, laid out the way glUniformMatrix4fv expects.
struct Mat4 {
    float m[16];
};

Mat4 identity() {
    Mat4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

Mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
    Mat4 r{};
    float f = 1.0f / std::tan(fovY / 2.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zFar + zNear) / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = 2.0f * zFar * zNear / (zNear - zFar);
    return r;
}

Mat4 translate(float x, float y, float z) {
    Mat4 r = identity();
    r.m[12] = x;
    r.m[13] = y;
    r.m[14] = z;
    return r;
}

Mat4 rotateY(float a) {
    Mat4 r = identity();
    r.m[0] = std::cos(a);
    r.m[2] = -std::sin(a);
    r.m[8] = std::sin(a);
    r.m[10] = std::cos(a);
    return r;
}

Mat4 rotateX(float a) {
    Mat4 r = identity();
    r.m[5] = std::cos(a);
    r.m[6] = std::sin(a);
    r.m[9] = -std::sin(a);
    r.m[10] = std::cos(a);
    return r;
}

// Unit sphere: position and normal are the same vector, so each vertex
// stores just (x, y, z) and the shader reuses it as the normal.
void buildSphere(int stacks, int slices, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * i / stacks;  // 0 at the north pole, PI at the south
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * PI * j / slices;
            vertices.push_back(std::sin(phi) * std::cos(theta));
            vertices.push_back(std::cos(phi));
            vertices.push_back(std::sin(phi) * std::sin(theta));
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int a = i * (slices + 1) + j;
            unsigned int b = a + slices + 1;
            indices.insert(indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint compileShaderProgram() {
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;
        out vec3 vWorldPos;
        out vec3 vNormal;
        void main() {
            vec4 world = uModel * vec4(aPos, 1.0);
            vWorldPos = world.xyz;
            // The model matrix is a pure rotation, so it transforms normals
            // correctly without an inverse-transpose.
            vNormal = mat3(uModel) * aPos;
            gl_Position = uProjection * uView * world;
        }
    )";

    // Phong reflection model, evaluated per fragment (Phong shading).
    const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec3 vWorldPos;
        in vec3 vNormal;
        out vec4 FragColor;

        uniform vec3 uLightPos;
        uniform vec3 uViewPos;
        uniform vec3 uLightColor;     // Il
        uniform vec3 uAmbientLight;   // Ia
        uniform vec3 uObjectColor;
        uniform float uKa;
        uniform float uKd;
        uniform float uKs;
        uniform float uShininess;     // n
        uniform int uMode;            // 0 ambient, 1 diffuse, 2 specular, 3 all

        void main() {
            vec3 N = normalize(vNormal);
            vec3 L = normalize(uLightPos - vWorldPos);
            vec3 V = normalize(uViewPos - vWorldPos);
            vec3 R = reflect(-L, N);

            vec3 ambient = uKa * uAmbientLight * uObjectColor;
            vec3 diffuse = uKd * max(dot(N, L), 0.0) * uLightColor * uObjectColor;
            // No highlight on the side facing away from the light.
            float spec = dot(N, L) > 0.0 ? pow(max(dot(R, V), 0.0), uShininess) : 0.0;
            vec3 specular = uKs * spec * uLightColor;

            vec3 color;
            if (uMode == 0) color = ambient;
            else if (uMode == 1) color = diffuse;
            else if (uMode == 2) color = specular;
            else color = ambient + diffuse + specular;

            FragColor = vec4(min(color, vec3(1.0)), 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(fragmentShader, sizeof(log), nullptr, log);
        std::cerr << "Fragment shader compilation failed:\n" << log << "\n";
    }

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

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lab7 - Phong Illumination Model", nullptr, nullptr);
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

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    buildSphere(64, 96, vertices, indices);

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);

    // Camera at (0, 0, 3.2) looking down -Z at the sphere.
    const float viewPos[3] = {0.0f, 0.0f, 3.2f};
    Mat4 view = translate(-viewPos[0], -viewPos[1], -viewPos[2]);
    Mat4 projection = perspective(45.0f * PI / 180.0f, 1.0f, 0.1f, 100.0f);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, projection.m);
    glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, viewPos);
    glUniform3f(glGetUniformLocation(shaderProgram, "uLightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "uAmbientLight"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "uObjectColor"), 0.85f, 0.30f, 0.25f);
    glUniform1f(glGetUniformLocation(shaderProgram, "uKa"), 0.15f);
    glUniform1f(glGetUniformLocation(shaderProgram, "uKd"), 0.75f);
    glUniform1f(glGetUniformLocation(shaderProgram, "uKs"), 0.6f);
    glUniform1f(glGetUniformLocation(shaderProgram, "uShininess"), 32.0f);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "uLightPos");
    GLint modeLoc = glGetUniformLocation(shaderProgram, "uMode");

    int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // The light orbits the sphere at radius 3, above and in front of it.
        // It holds still for the first frames so the screenshot is
        // reproducible.
        float lightAngle = 0.7f + std::max(0, frame - 10) * 0.01f;
        float lightPos[3] = {3.0f * std::sin(lightAngle), 2.0f, 3.0f * std::cos(lightAngle)};
        Mat4 model = multiply(rotateY(0.4f), rotateX(0.3f));

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.m);
        glUniform3fv(lightPosLoc, 1, lightPos);

        // 2x2 grid: top-left ambient, top-right diffuse, bottom-left
        // specular, bottom-right the full Phong sum.
        int halfW = fbWidth / 2, halfH = fbHeight / 2;
        const int panels[4][2] = {{0, halfH}, {halfW, halfH}, {0, 0}, {halfW, 0}};
        for (int mode = 0; mode < 4; ++mode) {
            glViewport(panels[mode][0], panels[mode][1], halfW, halfH);
            glUniform1i(modeLoc, mode);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
        }

        if (frame == 3) {
            saveScreenshotPPM("screenshots/phong_illumination.ppm", fbWidth, fbHeight);
        }
        ++frame;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
