# Lab 7 — Phong Illumination Model

`phong.cpp` lights a 3D sphere with the Phong reflection model, evaluated
per fragment in the GLSL fragment shader:

`I = Ia·ka + Il·kd·max(N·L, 0) + Il·ks·max(R·V, 0)ⁿ`

- **N** — surface normal (the sphere is generated procedurally, so each
  vertex's position doubles as its normal)
- **L** — unit vector from the surface point to the light
- **V** — unit vector from the surface point to the viewer
- **R** — `reflect(-L, N)`, the mirror direction of the light about N

Material: `ka = 0.15`, `kd = 0.75`, `ks = 0.6`, shininess `n = 32`, with a
white light and a red object color. The specular term uses the light color
only, so the highlight is white.

The window is split into four viewports rendering the same sphere:

| Top-left: ambient only | Top-right: diffuse only |
|---|---|
| **Bottom-left: specular only** | **Bottom-right: full Phong (sum)** |

The light orbits the sphere, so the diffuse falloff and the specular
highlight move across the surface.

## Build and run

```sh
cmake -S . -B build
cmake --build build
./build/phong
```

The program automatically saves a screenshot
(`screenshots/phong_illumination.ppm`) a few frames after launch, then keeps
the window open — press <kbd>Esc</kbd> or close it to exit. Convert to PNG
with:

```sh
cd screenshots
sips -s format png phong_illumination.ppm --out phong_illumination.png
```

The final screenshot is `screenshots/phong_illumination.png`.
