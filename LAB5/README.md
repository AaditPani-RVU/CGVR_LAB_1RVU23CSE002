# Lab 5 — Cohen–Sutherland Line Clipping Algorithm

`cohen_sutherland.cpp` clips five test line segments against a clip window,
covering the algorithm's standard cases:

- a line fully inside the window (trivial accept, no clipping needed)
- a line fully outside the window (trivial reject)
- a line crossing the left and right edges
- a line crossing the top and bottom edges
- a diagonal line crossing a corner region (clipped against two edges)

Each candidate line is classified with a 4-bit outcode (`LEFT | RIGHT |
BOTTOM | TOP`) relative to the clip window, then clipped one edge at a time
until it is trivially accepted or rejected.

The white rectangle is the clip window. Accepted lines are drawn twice: their
original extent in steel blue, and the clipped-in portion on top in bright
green — so you can see exactly what the algorithm cut away. The rejected
line is drawn once, in dim red, off on its own outside the window.

## Build and run

```sh
cmake -S . -B build
cmake --build build
./build/cohen_sutherland
```

The program automatically saves a screenshot
(`screenshots/cohen_sutherland.ppm`) a few frames after launch, then keeps
the window open — press <kbd>Esc</kbd> or close it to exit. Convert to PNG
with:

```sh
cd screenshots
sips -s format png cohen_sutherland.ppm --out cohen_sutherland.png
```

The final screenshot is `screenshots/cohen_sutherland.png`.
