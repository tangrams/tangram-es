#pragma once

#include <vector>
#include <cmath>

#include "glm/glm.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"

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
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

/* Maximum value between two values that support > comparison */
#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

/* Clamp a value between a min and max value */
#ifndef CLAMP
#define CLAMP(val,min,max) ((val) < (min) ? (min) : ((val > max) ? (max) : (val)))
#endif

/* Absolute value of a numeric variable */
#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

/* Returns 1 for positive values, -1 for negative values and 0 otherwise.
 * Ex: signValue(-2) == -1
 */
int signValue(float _n);

/* Returns an equivalent angle in radians between -PI and PI
 * Ex: wrapRad(3.24159265358979323846) == -0.1
 * Ex: wrapRad(6.28318530717958647693) == 0.0
 */
void wrapRad(double& _rads);

/* Map a value from the range [_inputMin, _inputMax] into the range [_outputMin, _outputMax];
 * If _clamp is true, the output is strictly within the output range.
 * Ex: mapValue(5, 0, 10, 0, 360) == 180
 */
float mapValue(const float& _value, const float& _inputMin, const float& _inputMax, const float& _outputMin, const float& _outputMax, bool _clamp = true);

/* Returns linear interpolation between _start and _stop by the fraction _amt */
float lerp(const float& _start, const float& _stop, float const& _amt);

/* Sets the length of _vec to _length */
void setLength(glm::vec3& _vec, float _length);

/* Gets a copy of _vec scaled to have length _length */
glm::vec3 getWithLength(const glm::vec3& _vec, float _length);

bool isPowerOf2(unsigned int _val);

/* Computes the angle in radians between two points with the axis y = 0 in 2d space */
float angleBetweenPoints(const glm::vec2& _p1, const glm::vec2& _p2);

/* Computes the clip coordinates from position in world space and a model view matrix */
glm::vec4 worldToClipSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition);

/* Computes the screen coordinates from a coordinate in clip space and a screen size */
glm::vec2 clipToScreenSpace(const glm::vec4& _clipCoords, const glm::vec2& _screenSize);

/* Computes the screen coordinates from a world position, a model view matrix and a screen size */
glm::vec2 worldToScreenSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition, const glm::vec2& _screenSize);
