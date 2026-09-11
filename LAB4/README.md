# Lab 4 — Composite 2D Transformations

This program demonstrates composite transformations using 3×3 homogeneous
coordinate matrices and column vectors. The shape is scaled, rotated, then
translated:

`M = T(220, 90) × R(35°) × S(0.75, 0.75)`

The screenshot shows the original L-shaped polygon in gray and the final
transformed polygon in cyan. Orange squares mark the final vertices.

## Build and run

```sh
cmake -S . -B build
cmake --build build
mkdir -p screenshots
./build/composite_transformations \
  screenshots/composite_transformations.ppm \
  screenshots/composite_matrix.txt
sips -s format png screenshots/composite_transformations.ppm \
  --out screenshots/composite_transformations.png
```

The `screenshots` directory contains the rendered PNG, its raw PPM render, and
a text report with the composite matrix and transformed vertex coordinates.
