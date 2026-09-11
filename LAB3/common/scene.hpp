#pragma once

#include <vector>

#include "transform2d.hpp"

// An asymmetric L-shaped polygon (pixel-space, roughly centered on the
// origin) so translation, rotation, reflection and shearing are all visually
// distinguishable from the original.
inline std::vector<Point2> sampleShape() {
    return {{-100, -100}, {60, -100}, {60, -40}, {140, -40}, {140, 100}, {-100, 100}};
}

// X and Y axis line segments through the origin, for visual reference.
inline std::vector<Point2> axisLines() {
    return {{-380, 0}, {380, 0}, {0, -280}, {0, 280}};
}
