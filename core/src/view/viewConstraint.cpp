#include "viewConstraint.h"

namespace Tangram {

void ViewConstraint::setLimitsY(double _yMin, double _yMax) {

    m_yMin = _yMin;
    m_yMax = _yMax;

}

void ViewConstraint::setLimitsX(double _xMin, double _xMax) {

    m_xMin = _xMin;
    m_xMax = _xMax;

}

void ViewConstraint::setRadius(double _r) {

    m_radius = _r;

}

auto ViewConstraint::getConstrainedX(double _x) -> double {

    return constrain(_x, m_radius, m_xMin, m_xMax);

}

auto ViewConstraint::getConstrainedY(double _y) -> double {

    return constrain(_y, m_radius, m_yMin, m_yMax);

}

auto ViewConstraint::getConstrainedScale() -> double {

    double xScale = 1.0, yScale = 1.0;
    double xRange = m_xMax - m_xMin;
    double yRange = m_yMax - m_yMin;
    double diameter = 2.0 * m_radius;

    if (diameter > yRange) { yScale = yRange / diameter; }
    if (diameter > xRange) { xScale = xRange / diameter; }

    return std::fmin(xScale, yScale);

}

auto ViewConstraint::constrain(double _pos, double _radius, double _min, double _max) -> double {

    double spaceAbove = _max - (_pos + _radius);
    double spaceBelow = (_pos - _radius) - _min;

    if (spaceAbove < 0.0 && spaceBelow < 0.0) {
        return 0.5 * (_max + _min);
    } else if (spaceAbove < 0.0) {
        return _max - _radius;
    } else if (spaceBelow < 0.0) {
        return _min + _radius;
    }

    return _pos;

}

}
