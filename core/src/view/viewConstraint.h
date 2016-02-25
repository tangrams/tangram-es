#pragma once
#include <cmath>

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
