#include "view/viewConstraint.h"

namespace Tangram {

void ViewConstraint::setLimitsY(double yMin, double yMax) {

    m_yMin = yMin;
    m_yMax = yMax;

}

void ViewConstraint::setLimitsX(double xMin, double xMax) {

    m_xMin = xMin;
    m_xMax = xMax;

}

void ViewConstraint::setRadius(double r) {

    m_radius = r;

}

auto ViewConstraint::getConstrainedX(double x) -> double {

    return constrain(x, m_radius, m_xMin, m_xMax);

}

auto ViewConstraint::getConstrainedY(double y) -> double {

    return constrain(y, m_radius, m_yMin, m_yMax);

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

auto ViewConstraint::constrain(double pos, double radius, double min, double max) -> double {

    double upperSpace = max - (pos + radius);
    double lowerSpace = (pos - radius) - min;

    if (upperSpace < 0.0 && lowerSpace < 0.0) {
        return 0.5 * (max + min);
    } else if (upperSpace < 0.0) {
        return max - radius;
    } else if (lowerSpace < 0.0) {
        return min + radius;
    }

    return pos;

}

}
