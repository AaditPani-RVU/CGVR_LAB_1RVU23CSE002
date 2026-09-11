#pragma once

#include <cmath>
#include <vector>

// 2D point/vector used with 3x3 homogeneous transformation matrices.
struct Point2 {
    float x, y;
};

// Row-major 3x3 matrix operating on homogeneous coordinates (x, y, 1).
struct Mat3 {
    float m[3][3];
};

inline Point2 applyTransform(const Mat3& m, const Point2& p) {
    float hx = p.x, hy = p.y, hw = 1.0f;
    float x = m.m[0][0] * hx + m.m[0][1] * hy + m.m[0][2] * hw;
    float y = m.m[1][0] * hx + m.m[1][1] * hy + m.m[1][2] * hw;
    return {x, y};
}

inline std::vector<Point2> applyTransform(const Mat3& m, const std::vector<Point2>& pts) {
    std::vector<Point2> out;
    out.reserve(pts.size());
    for (const Point2& p : pts) out.push_back(applyTransform(m, p));
    return out;
}

// Homogeneous-coordinate translation: [x' y' 1]^T = T(tx,ty) * [x y 1]^T
inline Mat3 translationMatrix(float tx, float ty) {
    return Mat3{{{1, 0, tx}, {0, 1, ty}, {0, 0, 1}}};
}

// Rotation about the origin by angleDegrees (counter-clockwise).
inline Mat3 rotationMatrix(float angleDegrees) {
    float rad = angleDegrees * static_cast<float>(M_PI) / 180.0f;
    float c = std::cos(rad), s = std::sin(rad);
    return Mat3{{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
}

// Scaling about the origin.
inline Mat3 scalingMatrix(float sx, float sy) {
    return Mat3{{{sx, 0, 0}, {0, sy, 0}, {0, 0, 1}}};
}

// Reflection about the X axis (y -> -y).
inline Mat3 reflectionMatrixX() {
    return Mat3{{{1, 0, 0}, {0, -1, 0}, {0, 0, 1}}};
}

// Reflection about the Y axis (x -> -x).
inline Mat3 reflectionMatrixY() {
    return Mat3{{{-1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
}

// Shearing: shx shears along X proportional to y, shy shears along Y proportional to x.
inline Mat3 shearingMatrix(float shx, float shy) {
    return Mat3{{{1, shx, 0}, {shy, 1, 0}, {0, 0, 1}}};
}
