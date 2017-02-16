#pragma once

#include "glm/vec2.hpp"
#include <cmath>
#include <functional>

namespace Tangram {
namespace Rasterize {

using ScanCallback = std::function<void (int, int)>;

struct Edge { // An edge between two points; oriented such that y is non-decreasing
    double x0 = 0, y0 = 0;
    double x1 = 0, y1 = 0;
    double dx = 0, dy = 0;
    Edge(glm::dvec2 _a, glm::dvec2 _b);
};

void scanLine(int _x0, int _x1, int _y, const ScanCallback& _s);

void scanSpan(Edge _e0, Edge _e1, int _min, int _max, const ScanCallback& _s);

void scanTriangle(glm::dvec2& _a, glm::dvec2& _b, glm::dvec2& _c, int _min, int _max, const ScanCallback& _s);

}
}
