#pragma once
#include <cmath>

namespace Tangram {

class ViewConstraint {

public:

    void setLimitsY(double _yMin, double _yMax);
    void setLimitsX(double _xMin, double _xMax);
    void setRadius(double _r);
    auto getConstrainedX(double _x) -> double;
    auto getConstrainedY(double _y) -> double;
    auto getConstrainedScale() -> double;

private:

    auto constrain(double _pos, double _radius, double _min, double _max) -> double;

    double m_xMax = INFINITY;
    double m_yMax = INFINITY;
    double m_xMin = -INFINITY;
    double m_yMin = -INFINITY;
    double m_radius = 0.0;

};

}
