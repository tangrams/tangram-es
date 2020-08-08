#pragma once

#include "glm/vec2.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

namespace Tangram {

constexpr double PI = 3.14159265358979323846;
constexpr double TWO_PI = 6.28318530717958647693;
constexpr double FOUR_PI = 12.56637061435917295385;
constexpr double HALF_PI = 1.57079632679489661923;

/// Multiply degrees by DEG_TO_RAD to get radians.
constexpr double DEG_TO_RAD = 0.01745329251994329576;

/// Multiply radians by RAD_TO_DEG to get degrees.
constexpr double RAD_TO_DEG = 57.2957795130823208767;

/// Minimum value between two variables that support < comparison.
template<typename T>
constexpr T min(T a, T b) {
    return a < b ? a : b;
}

/// Maximum value between two values that support > comparison.
template<typename T>
constexpr T max(T a, T b) {
    return a > b ? a : b;
}

/// Clamp a value between a min and max value.
template<typename T>
constexpr T clamp(T value, T min, T max) {
    return value < min ? min : (value > max ? max : value);
}

/// Clamp a value to the range [0, 1].
template<typename T>
constexpr T clamp01(T value) {
    return clamp(value, 0.f, 1.f);
}

/// Absolute value of a numeric variable.
template<typename T>
constexpr T abs(T value) {
    return value < 0 ? -value : value;
}

struct BoundingBox {

    glm::dvec2 min;
    glm::dvec2 max;

    double width() const { return glm::abs(max.x - min.x); }
    double height() const { return glm::abs(max.y - min.y); }
    glm::dvec2 center() const { return 0.5 * (min + max); }
    bool containsX(double x) const { return x >= min.x && x <= max.x; }
    bool containsY(double y) const { return y >= min.y && y <= max.y; }
    bool contains(double x, double y) const { return containsX(x) && containsY(y); }
    void expand(double x, double y) {
        min = { glm::min(min.x, x), glm::min(min.y, y) };
        max = { glm::max(max.x, x), glm::max(max.y, y) };
    }
};

/// Return the signed area of the polygon defined by the points in the range [begin, end).
/// The sign of the area is positive if the vertices of the polygon are counter-clockwise.
template<class InputIt>
float signedArea(InputIt begin, InputIt end) {
    if (begin == end) { return 0; }
    float area = 0;
    auto prev = end - 1;
    for (auto curr = begin; curr != end; ++curr) {
        area += curr->x * prev->y - curr->y * prev->x;
        prev = curr;
    }
    return 0.5f * area;
}

/// Calculate the area centroid of a closed polygon given as a sequence of vectors.
/// If the polygon has no area, the coordinates returned are NaN.
template<class InputIt, class Vector = typename InputIt::value_type>
Vector centroid(InputIt begin, InputIt end) {
    Vector centroid;
    float area = 0.f;
    const Vector offset(begin->x, begin->y);

    for (auto curr = begin, prev = end - 1; curr != end; prev = curr, ++curr) {
        const Vector prevPoint(prev->x - offset.x, prev->y - offset.y);
        const Vector currPoint(curr->x - offset.x, curr->y - offset.y);
        float a = (prevPoint.x * currPoint.y - currPoint.x * prevPoint.y);
        centroid.x += (prevPoint.x + currPoint.x) * a;
        centroid.y += (prevPoint.y + currPoint.y) * a;
        area += a;
    }
    centroid.x = centroid.x / (3.f * area) + offset.x;
    centroid.y = centroid.y / (3.f * area) + offset.y;
    return centroid;
}

/// Return the signed area of the triangle formed by the points a, b, and c in the plane.
template<class T>
float signedArea(const T& a, const T& b, const T& c) {
    return 0.5 * ((b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y));
}

/// Return the 'perpendicular dot product' of two vectors in the plane.
/// The result is equal to |a|*|b|*sin(theta), where theta is the angle between the vectors.
/// https://mathworld.wolfram.com/PerpDotProduct.html
inline float perpDotProduct(const glm::vec2& a, const glm::vec2& b) {
    return (a.x * b.y) - (a.y * b.x);
}

/// Map a value from the range [inputMin, inputMax] into the range [0, 1].
/// Ex: mapRange01(6, 2, 10) == 0.5, mapRange01(10, 0, 5) == 1
float mapRange01(float value, float inputMin, float inputMax);

/// Computes the clip coordinates from position in world space and a model view matrix
inline glm::vec4 worldToClipSpace(const glm::mat4& mvp, const glm::vec4& worldPosition) {
    return mvp * worldPosition;
}

/// Return true if the given clip space point is behind the camera plane.
inline bool clipSpaceIsBehindCamera(const glm::vec4& clip) {
    return clip.w < 0;
}

/// Return the given clip space point converted to Normalized Device Coordinates.
inline glm::vec3 clipSpaceToNdc(const glm::vec4& clip) {
    return glm::vec3(clip) / clip.w;
}

/// Return the given point in Normalized Device Coordinates converted to screen space.
inline glm::vec2 ndcToScreenSpace(const glm::vec3& ndc, const glm::vec2& screenSize) {
    return glm::vec2(1 + ndc.x, 1 - ndc.y) * screenSize * 0.5f;
}

/// Compute the screen coordinates from a world position, a model view matrix and a screen size.
glm::vec2 worldToScreenSpace(const glm::mat4& mvp, const glm::vec4& worldPosition, const glm::vec2& screenSize, bool& behindCamera);

/// Return the rotation unit vector representing a counter-clockwise rotation in the plane of the given radians.
inline glm::vec2 rotation2dRad(float radians) {
    return { cos(radians), sin(radians) };
}

/// Return the rotation unit vector representing a counter-clockwise rotation in the plane of the given degrees.
inline glm::vec2 rotation2dDeg(float degrees) {
    return rotation2dRad(static_cast<float>(DEG_TO_RAD) * degrees);
}

/// Return the vector 'in' rotated in the plane by the rotation unit vector 'rotation'.
inline glm::vec2 rotate2d(const glm::vec2& in, const glm::vec2& rotation) {
    return { in.x * rotation.x + in.y * rotation.y, -in.x * rotation.y + in.y * rotation.x };
}

/// Return true if the value is exactly a power of two.
inline bool isPowerOfTwo(int value) {
    return (value & (value - 1)) == 0;
}

/// Return the square of the distance from the point 'p' to the nearest point on the segment from 'a' to 'b'.
float pointSegmentDistanceSq(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b);

/// Return the distance from the point 'p' to the nearest point on the segment from 'a' to 'b'.
float pointSegmentDistance(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b);

}
