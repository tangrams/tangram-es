#pragma once

#include <vector>

namespace Tangram {

struct Frame {
    float key = 0;
    union {
        float value;
        uint32_t color = 0;
    };
    Frame(float _k, float _v) : key(_k), value(_v) {}
    Frame(float _k, uint32_t _c) : key(_k), color(_c) {}
};

struct Stops {

    std::vector<Frame> frames;

    auto evalFloat(float _key) const -> float;
    auto evalColor(float _key) const -> uint32_t;
    auto nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator;
};

}
