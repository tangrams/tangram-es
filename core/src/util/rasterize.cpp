#include "util/rasterize.h"

namespace Tangram {
namespace Rasterize {

// Triangle rasterization adapted from Polymaps: https://github.com/simplegeo/polymaps/blob/master/src/Layer.js#L333-L383

Edge::Edge(glm::dvec2 _a, glm::dvec2 _b) {
    if (_a.y > _b.y) { std::swap(_a, _b); }
    x0 = _a.x;
    y0 = _a.y;
    x1 = _b.x;
    y1 = _b.y;
    dx = x1 - x0;
    dy = y1 - y0;
}

void scanLine(int _x0, int _x1, int _y, const ScanCallback& _s) {
    for (int x = _x0; x < _x1; x++) {
        _s(x, _y);
    }
}

void scanSpan(Edge _e0, Edge _e1, int _min, int _max, const ScanCallback& _s) {

    // _e1 has a shorter y-span, so we'll use it to limit our y coverage
    int y0 = fmax(_min, floor(_e1.y0));
    int y1 = fmin(_max, ceil(_e1.y1));

    // sort edges by x-coordinate
    if (_e0.x0 == _e1.x0 && _e0.y0 == _e1.y0) {
        if (_e0.x0 + _e1.dy / _e0.dy * _e0.dx < _e1.x1) { std::swap(_e0, _e1); }
    } else {
        if (_e0.x1 - _e1.dy / _e0.dy * _e0.dx < _e1.x0) { std::swap(_e0, _e1); }
    }

    // scan lines!
    double m0 = _e0.dx / _e0.dy;
    double m1 = _e1.dx / _e1.dy;
    double d0 = _e0.dx > 0 ? 1.0 : 0.0;
    double d1 = _e1.dx < 0 ? 1.0 : 0.0;
    for (int y = y0; y < y1; y++) {
        double x0 = m0 * fmax(0.0, fmin(_e0.dy, y + d0 - _e0.y0)) + _e0.x0;
        double x1 = m1 * fmax(0.0, fmin(_e1.dy, y + d1 - _e1.y0)) + _e1.x0;
        scanLine(floor(x1), ceil(x0), y, _s);
    }

}

void scanTriangle(glm::dvec2& _a, glm::dvec2& _b, glm::dvec2& _c, int _min, int _max, const ScanCallback& _s) {

    Edge ab = Edge(_a, _b);
    Edge bc = Edge(_b, _c);
    Edge ca = Edge(_c, _a);

    // place edge with greatest y distance in ca
    if (ab.dy > ca.dy) { std::swap(ab, ca); }
    if (bc.dy > ca.dy) { std::swap(bc, ca); }

    // scan span! scan span!
    if (ab.dy > 0) { scanSpan(ca, ab, _min, _max, _s); }
    if (bc.dy > 0) { scanSpan(ca, bc, _min, _max, _s); }

}

}
}
