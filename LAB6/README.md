# Lab 6 — Z-Buffer Algorithm for Hidden Surface Removal

`z_buffer.cpp` implements the Z-buffer (depth-buffer) algorithm in software.
The scene is two interpenetrating cubes with a tilted plane slicing through
both of them — a case that no back-to-front ordering of whole polygons
(painter's algorithm) can draw correctly.

Every triangle is projected with a perspective camera and scan-converted on
the CPU:

1. The color buffer is cleared to the background and every Z-buffer entry is
   set to the far plane (`1.0`).
2. For each pixel a triangle covers, its depth is interpolated from the
   vertex depths with barycentric weights.
3. If that depth is less than the stored value, the pixel's color and depth
   are overwritten; otherwise the fragment is hidden and discarded.

No back-face culling is used, so the Z-buffer alone decides visibility.
OpenGL only displays the two software buffers side by side: the shaded color
buffer on the left and the depth buffer on the right (nearer surfaces are
brighter, empty pixels are black).

## Build and run

```sh
cmake -S . -B build
cmake --build build
./build/z_buffer
```

Press <kbd>Space</kbd> to toggle the depth test on and off, and
<kbd>Esc</kbd> to exit. With the test off, triangles simply overwrite each
other in submission order (back cube, front cube, plane), so the plane wrongly
covers both cubes.

The program automatically saves two screenshots a few frames after launch —
`screenshots/z_buffer.ppm` (depth test on) and
`screenshots/without_z_buffer.ppm` (depth test off) — then keeps the window
open with the front cube slowly spinning. Convert them to PNG with:

```sh
cd screenshots
sips -s format png z_buffer.ppm --out z_buffer.png
sips -s format png without_z_buffer.ppm --out without_z_buffer.png
```

The final screenshots are `screenshots/z_buffer.png` and
`screenshots/without_z_buffer.png`.
