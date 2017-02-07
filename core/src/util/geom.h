#pragma once

#include "glm/glm.hpp"
#include <vector>

#ifndef PI
#define PI 3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647693
#endif

#ifndef FOUR_PI
#define FOUR_PI 12.56637061435917295385
#endif

#ifndef HALF_PI
#define HALF_PI 1.57079632679489661923
#endif

#ifndef QUARTER_PI
#define QUARTER_PI 0.785398163
#endif

/* Multiply degrees by DEG_TO_RAD to get radians */
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.01745329251994329576
#endif

/* Multiply radians by RAD_TO_DEG to get degrees */
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.2957795130823208767
#endif

/* Minimum value between two variables that support < comparison */
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

/* Maximum value between two values that support > comparison */
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

/* Clamp a value between a min and max value */
#ifndef CLAMP
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val > max) ? (max) : (val)))
#endif

/* Absolute value of a numeric variable */
#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

namespace Tangram {

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

template<class InputIt>
float signedArea(InputIt _begin, InputIt _end) {
    if (_begin == _end) { return 0; }
    float area = 0;
    auto prev = _end - 1;
    for (auto curr = _begin; curr != _end; ++curr) {
        area += curr->x * prev->y - curr->y * prev->x;
        prev = curr;
    }
    return 0.5 * area;
}

/* Calculate the area centroid of a closed polygon given as a sequence of vectors.
 * If the polygon has no area, the coordinates returned are NaN.
 */
template<class InputIt, class Vector = typename InputIt::value_type>
Vector centroid(InputIt begin, InputIt end) {
    Vector centroid;
    float area = 0.f;
    for (auto curr = begin, prev = end - 1; curr != end; prev = curr, ++curr) {
        float a = (prev->x * curr->y - curr->x * prev->y);
        centroid.x += (prev->x + curr->x) * a;
        centroid.y += (prev->y + curr->y) * a;
        area += a;
    }
    return centroid / (3.f * area);
}

template<class T>
float signedArea(const T& _a, const T& _b, const T& _c) {
    return 0.5 * ((_b.y - _a.y) * (_c.x - _b.x) - (_b.x - _a.x) * (_c.y - _b.y));
}

inline float crossProduct(const glm::vec2& _a, const glm::vec2& _b) {
    return (_a.x * _b.y) - (_a.y * _b.x);
}

/* Map a value from the range [_inputMin, _inputMax] into the range [_outputMin, _outputMax];
 * If _clamp is true, the output is strictly within the output range.
 * Ex: mapValue(5, 0, 10, 0, 360) == 180
 */
float mapValue(const float& _value, const float& _inputMin, const float& _inputMax, const float& _outputMin,
               const float& _outputMax, bool _clamp = true);

/* Computes the angle in radians between two points with the axis y = 0 in 2d space */
float angleBetweenPoints(const glm::vec2& _p1, const glm::vec2& _p2);

/* Computes the clip coordinates from position in world space and a model view matrix */
glm::vec4 worldToClipSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition);

/* Computes the screen coordinates from a coordinate in clip space and a screen size */
glm::vec2 clipToScreenSpace(const glm::vec4& _clipCoords, const glm::vec2& _screenSize);

/* Computes the screen coordinates from a world position, a model view matrix and a screen size */
glm::vec2 worldToScreenSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition, const glm::vec2& _screenSize);
glm::vec2 worldToScreenSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition, const glm::vec2& _screenSize, bool& clipped);

glm::vec2 worldToScreenSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition, const glm::vec2& _screenSize, bool& _clipped);

/* Computes the geometric center of the two dimensional region defined by the polygon */
glm::vec2 centroid(const std::vector<std::vector<glm::vec3>>& _polygon);

inline glm::vec2 rotateBy(const glm::vec2& _in, const glm::vec2& _normal) {
    return {
        _in.x * _normal.x + _in.y * _normal.y,
        -_in.x * _normal.y + _in.y * _normal.x
    };
}

float sqSegmentDistance(const glm::vec2& _p, const glm::vec2& _p1, const glm::vec2& _p2);

bool isPowerOfTwo(int _value);

float sqPointSegmentDistance(const glm::vec2& _p, const glm::vec2& _a, const glm::vec2& _b);
float pointSegmentDistance(const glm::vec2& _p, const glm::vec2& _a, const glm::vec2& _b);

}
