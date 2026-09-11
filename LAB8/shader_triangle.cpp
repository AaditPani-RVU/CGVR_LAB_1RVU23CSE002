#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>

namespace {

constexpr int kWidth = 800;
constexpr int kHeight = 600;

const char* kVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPosition;
    layout (location = 1) in vec3 aColor;

    out vec3 vertexColor;

    void main() {
        gl_Position = vec4(aPosition, 0.0, 1.0);
        vertexColor = aColor;
    }
)";

const char* kFragmentShaderSource = R"(
    #version 330 core
    in vec3 vertexColor;
    out vec4 fragmentColor;

    void main() {
        fragmentColor = vec4(vertexColor, 1.0);
    }
)";

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_TRUE) return shader;

    char log[1024]{};
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::cerr << "Shader compilation failed:\n" << log << '\n';
    glDeleteShader(shader);
    return 0;
}

GLuint buildShaderProgram() {
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, kVertexShaderSource);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
    if (!vertex || !fragment) return 0;

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_TRUE) return program;

    char log[1024]{};
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    std::cerr << "Shader program linking failed:\n" << log << '\n';
    glDeleteProgram(program);
    return 0;
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

}  // namespace

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Lab 8 - Vertex and Fragment Shaders", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // position (x, y), then RGB color: the fragment shader interpolates color.
    const float vertices[] = {
        -0.75F, -0.65F, 1.0F, 0.15F, 0.20F,
         0.75F, -0.65F, 0.20F, 1.0F, 0.35F,
         0.00F,  0.75F, 0.20F, 0.45F, 1.0F,
    };

    const GLuint program = buildShaderProgram();
    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.025F, 0.05F, 0.10F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
