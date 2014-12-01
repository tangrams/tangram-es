#pragma once

#include <vector>
#include <math.h>
#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

//  Pre-Computed trigonometrical constant
//
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

//  Constat to convert angles between Radiants and Degrees
//-----------------------------------------------------------
//  Multiply a degree by DEG_TO_RAD to get a radiant.
//  Ex: 45*DEG_TO_RAD == QUARTER_PI
#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI/180.0)
#endif

//  Multiply a radiant bt RAD_TO_DEG to get a degree angle.
//  Ex: PI*RAD_TO_DEG == 180
#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180.0/PI)
#endif

//  Get the minimum or maximum values of a pair of variables
//-----------------------------------------------------------
//  Ex: MIN(2,3) == 2
#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

//  Ex: MAX(2,3) == 3
#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

//  Clamp a value between a min and max values
//  Ex: CLAMP(5,2,3) == 3
#ifndef CLAMP
#define CLAMP(val,min,max) ((val) < (min) ? (min) : ((val > max) ? (max) : (val)))
#endif

//  Get the absolute (unsigned) of a variable
//  Ex: ABS(-12) == 12 && ABS(12) == 12
#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

//  Return the sign of a variable. 1 (positive), -1 (negative) and 0 if the variable is 0.
//  Ex: signValue(-2) == -1
int         signValue(float _n);

//  Return a radiant angle between PI and -PI.
//  Ex: wrapRad(3.24159265358979323846) == -0.1
//  Ex: wrapRad(6.28318530717958647693) == 0.0
void        wrapRad(double &_angle);

//  Map a value that have a min and max into an variable witch it own min and max
//  Ex: mapValue(5,0,10,0,360) == 180
float       mapValue(const float &value, const float &inputMin, const float &inputMax, const float &outputMin, const float &outputMax, bool clamp = true);

//  Calculates a number between two numbers at a specific increment
float       lerpValue(const float &_start, const float &_stop, float const &_amt);

//  Scale a vector by a pct (0.0-1.0) of it lenght
void        scale(glm::vec3 &_vec, float _length);
glm::vec3   getScaled(const glm::vec3 &_vec, float _length);

//  Calculate the total area a set of points
float       getArea(const std::vector<glm::vec3> &_pts);

//  Calculate the centroid of a set of poitns
glm::vec3   getCentroid(const std::vector<glm::vec3> &_pts);

//  Douglas-Peucker line simplificationbased on a tolerance
void simplify(std::vector<glm::vec3> &_pts, float _tolerance=0.3f);
std::vector<glm::vec3> getSimplify(const std::vector<glm::vec3> &_pts, float _tolerance=0.3f);

//  Calculates teh convexHull of a set of points
std::vector<glm::vec3> getConvexHull(std::vector<glm::vec3> &_pts);
std::vector<glm::vec3> getConvexHull(const std::vector<glm::vec3> &_pts);
