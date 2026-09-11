# Lab 8 — Vertex and Fragment Shaders

`shader_triangle.cpp` is the Lab 8 OpenGL program. It creates an OpenGL 3.3
core window, compiles a vertex shader and a fragment shader, uploads a
position-and-color triangle to a VBO, and renders it through a VAO. The vertex
shader forwards per-vertex RGB values; the fragment shader receives the
interpolated color and writes it to the framebuffer.

`shader_preview.cpp` produces the matching reference render without a display
server. It is used to create the checked-in screenshot in headless execution
environments.

## Build and run

```sh
cmake -S . -B build
cmake --build build

# Interactive OpenGL application (press Esc to close):
./build/shader_triangle

# Deterministic screenshot generation:
mkdir -p screenshots
./build/shader_preview screenshots/vertex_fragment_shader.ppm
sips -s format png screenshots/vertex_fragment_shader.ppm \
  --out screenshots/vertex_fragment_shader.png
```

The screenshot shows the same red, green, and blue vertex colors interpolated
across the triangle by the fragment-shader stage.
