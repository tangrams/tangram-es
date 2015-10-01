#pragma once

#include "util/mapProjection.h"

#include <cstdint>
#include <vector>

namespace YAML {
    class Node;
}

namespace Tangram {

struct Stops {

    struct Frame {
        float key = 0;
        union {
            float value;
            uint32_t color = 0;
        };
        Frame(float _k, float _v) : key(_k), value(_v) {}
        Frame(float _k, uint32_t _c) : key(_k), color(_c) {}
    };

    std::vector<Frame> frames;
    static Stops Color(const YAML::Node& _node);
    static Stops Width(const YAML::Node& _node, const MapProjection& _projection);

    Stops(const std::vector<Frame>& _frames) : frames(_frames) {}
    Stops() {}

    auto evalFloat(float _key) const -> float;
    auto evalWidth(float _key) const -> float;
    auto evalColor(float _key) const -> uint32_t;
    auto nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator;
};

}
