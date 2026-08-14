# Lab01 — Bresenham line and midpoint circle

This project contains two separate 800×600 OpenGL programs:

- `bresenham`: two Bresenham lines and a midpoint circle.
- `dda`: two DDA lines.

## Run

From this folder, run:

```sh
cmake -S . -B build
cmake --build build
./build/bresenham
# or
./build/dda
```

The project uses the included GLAD source and requires GLFW 3.3 or later. On macOS with Homebrew:

```sh
brew install glfw cmake
```

Press <kbd>Esc</kbd> or close the window to exit.
