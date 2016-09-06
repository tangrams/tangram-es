#pragma once

#include <vector>

namespace Tangram {

struct Range {
    int start;
    int length;

    int end() const { return start + length; }

    Range(int _start, int _length) : start(_start), length(_length) {}

    Range() : start(0), length(0) {}
};

struct LngLat {
    LngLat() {}
    LngLat(double _lon, double _lat) : longitude(_lon), latitude(_lat) {}

    LngLat(const LngLat& _other) = default;
    LngLat(LngLat&& _other) = default;
    LngLat& operator=(const LngLat& _other) = default;
    LngLat& operator=(LngLat&& _other) = default;

    bool operator==(const LngLat& _other) {
        return longitude == _other.longitude &&
               latitude == _other.latitude;
    }

    double longitude = 0.0;
    double latitude = 0.0;
};

typedef std::vector<LngLat> Coordinates;

typedef uint32_t MarkerID;

}
