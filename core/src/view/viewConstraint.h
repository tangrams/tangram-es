#pragma once
#include <cmath>
#ifndef M_PI // M_PI is non-standard since c++99
#define M_PI (3.14159265358979323846264338327950288)
#endif

namespace Tangram {

class ViewConstraint {

public:

    void setLimitsY(double yMin, double yMax);
    void setLimitsX(double xMin, double xMax);
    void setRadius(double r);
    auto getConstrainedX(double x) -> double;
    auto getConstrainedY(double y) -> double;
    auto getConstrainedScale() -> double;

private:

    auto constrain(double pos, double radius, double min, double max) -> double;

    double m_xMax = INFINITY;
    double m_yMax = INFINITY;
    double m_xMin = -INFINITY;
    double m_yMin = -INFINITY;
    double m_radius = 0.0;

};

}
