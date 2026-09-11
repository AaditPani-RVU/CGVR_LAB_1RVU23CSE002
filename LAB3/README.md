# Lab 3 — 2D Transformations Using Homogeneous Coordinates

The lesson plan groups these five transformations under Lab 3:

- `translation.cpp` — translate by `(tx, ty)`
- `rotation.cpp` — rotate about the origin by 60°
- `scaling.cpp` — scale about the origin by `(1.6, 0.6)`
- `reflection.cpp` — reflect about the X axis and about the Y axis
- `shearing.cpp` — shear along X proportional to Y

Each program applies its transformation as a 3x3 homogeneous-coordinate
matrix (see `common/transform2d.hpp`) to an asymmetric L-shaped polygon, so
the effect of the transform is visually unambiguous. The original shape is
drawn in white, the transformed shape in a distinct color, with faint gray
axes through the origin for reference. `common/gl_app.hpp` holds the shared
GLFW/OpenGL window, shader and screenshot boilerplate reused by all five
programs.

## Build and run

```sh
cmake -S . -B build
cmake --build build
./build/translation
./build/rotation
./build/scaling
./build/reflection
./build/shearing
```

Each program automatically saves a screenshot (`screenshots/<name>.ppm`) a
few frames after launch, then keeps the window open — press <kbd>Esc</kbd> or
close it to exit. Convert the PPM captures to PNG with:

```sh
cd screenshots
for f in *.ppm; do sips -s format png "$f" --out "${f%.ppm}.png"; done
```

The final screenshots are the five PNG files in `screenshots/`.
