#pragma once

#include <vector>
#include <math.h>
#include <iostream>

#include "glm/glm.hpp"

#define GLM_FORCE_RADIANS
#include "glm/gtc/matrix_transform.hpp"

#ifndef PI
#define PI       3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI   6.28318530717958647693
#endif

#ifndef FOUR_PI
#define FOUR_PI 12.56637061435917295385
#endif

#ifndef HALF_PI
#define HALF_PI  1.57079632679489661923
#endif

#ifndef QUARTER_PI
#define QUARTER_PI 0.785398163
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI/180.0)
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180.0/PI)
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef CLAMP
#define CLAMP(val,min,max) ((val) < (min) ? (min) : ((val > max) ? (max) : (val)))
#endif

#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

int         signValue(float _n);
void        wrapRad(double &_angle);
float       mapValue(const float &value, const float &inputMin, const float &inputMax, const float &outputMin, const float &outputMax, bool clamp = true);
float       lerpValue(const float &_start, const float &_stop, float const &_amt);

void        scale(glm::vec3 &_vec, float _length);
glm::vec3   getScaled(const glm::vec3 &_vec, float _length);

float       getArea(const std::vector<glm::vec3> &_pts);
glm::vec3   getCentroid(const std::vector<glm::vec3> &_pts);

void simplify(std::vector<glm::vec3> &_pts, float _tolerance=0.3f);
std::vector<glm::vec3> getSimplify(const std::vector<glm::vec3> &_pts, float _tolerance=0.3f);

std::vector<glm::vec3> getConvexHull(std::vector<glm::vec3> &_pts);
std::vector<glm::vec3> getConvexHull(const std::vector<glm::vec3> &_pts);