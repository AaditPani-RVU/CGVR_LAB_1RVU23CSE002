# Lab 2 — Midpoint Circle Drawing Algorithm

The lesson plan places these related implementation sessions beneath Lab 2:

- `dda_line.cpp` — DDA line drawing
- `bresenham_line.cpp` — Bresenham line drawing
- `midpoint_circle.cpp` — midpoint circle drawing using `p = 1 - r` and
  eight-way symmetry

Each program rasterizes its own discrete pixels to a portable PPM image. The
circle output includes faint coordinate axes and an orange centre pixel.

## Build and run

```sh
cmake -S . -B build
cmake --build build
mkdir -p screenshots
./build/dda_line screenshots/dda_line.ppm
./build/bresenham_line screenshots/bresenham_line.ppm
./build/midpoint_circle screenshots/midpoint_circle.ppm
sips -s format png screenshots/dda_line.ppm --out screenshots/dda_line.png
sips -s format png screenshots/bresenham_line.ppm --out screenshots/bresenham_line.png
sips -s format png screenshots/midpoint_circle.ppm --out screenshots/midpoint_circle.png
```

The final screenshots are the three PNG files in `screenshots/`. Their PPM
counterparts are the raw rendered outputs used to create those screenshots.
