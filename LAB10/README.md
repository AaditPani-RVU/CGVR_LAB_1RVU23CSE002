# Lab 10 — 3D Viewing Pipeline (Projection and Viewport Mapping)

`viewing_pipeline.cpp` carries a wireframe 3D scene through every stage of
the viewing pipeline explicitly on the CPU, using 4×4 homogeneous matrices
(column vectors, `p' = M · p`):

```
object ──M──▶ world ──V──▶ eye ──P──▶ clip ──÷w──▶ NDC ──viewport──▶ screen
```

| Stage | What happens |
|---|---|
| **Modeling** `M` | Scale / rotate / translate each object into the world |
| **Viewing** `V` | `lookAt(eye, target, up)` builds the camera's `u, v, n` axes and moves the eye to the origin |
| **Clipping** | Edges are clipped against the near plane in eye space, so nothing behind the camera is projected |
| **Projection** `P` | Perspective (fov 45°, `w = −z_eye`) or orthographic (`w = 1`) |
| **Perspective divide** | `clip.xyz / clip.w` gives normalized device coordinates in `[−1, 1]` |
| **Viewport mapping** | `x_s = x_v + (x_ndc + 1)/2 · width`, `y_s = y_v + (y_ndc + 1)/2 · height` |

The same scene (a ground grid, a row of identical cubes receding into the
distance, a pyramid and a rotated cube) is shown through two 460×460
viewports on one window:

- **Left — perspective projection:** the identical cubes shrink with distance
  and parallel grid lines converge.
- **Right — orthographic projection:** every cube is the same size and
  parallel lines stay parallel.

OpenGL only draws the final 2D pixel-space segments (each view is scissored
to its viewport rectangle) — it never receives a 3D coordinate. After the
screenshot is taken, the camera slowly orbits the scene.

The program also writes `screenshots/pipeline_trace.txt`: the `M`, `V` and
both `P` matrices, plus one cube corner traced through object → world → eye
→ clip → NDC → screen for each projection.

## Build and run

```sh
cmake -S . -B build
cmake --build build
./build/viewing_pipeline
```

The program automatically saves a screenshot
(`screenshots/viewing_pipeline.ppm`) and the pipeline trace a few frames after
launch, then keeps the window open — press <kbd>Esc</kbd> or close it to
exit. Convert to PNG with:

```sh
cd screenshots
sips -s format png viewing_pipeline.ppm --out viewing_pipeline.png
```

The final screenshot is `screenshots/viewing_pipeline.png`.
