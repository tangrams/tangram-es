#pragma once

namespace Tangram {

struct GeoPoint {
    double x, y;
};

struct GeoLine {
    GeoPoint* points;
    int size;
};

struct GeoPolygon {
    GeoLine* lines;
    int size;
};

}
