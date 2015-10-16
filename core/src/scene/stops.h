#pragma once

#include "util/color.h"

#include <vector>

namespace YAML {
    class Node;
}

namespace Tangram {

class MapProjection;

struct Stops {

    struct Frame {
        float key = 0;
        union {
            float value;
            Color color;
        };
        Frame(float _k, float _v) : key(_k), value(_v) {}
        Frame(float _k, Color _c) : key(_k), color(_c) {}
    };

    std::vector<Frame> frames;
    static Stops Colors(const YAML::Node& _node);
    static Stops Widths(const YAML::Node& _node, const MapProjection& _projection);

    Stops(const std::vector<Frame>& _frames) : frames(_frames) {}
    Stops() {}

    auto evalFloat(float _key) const -> float;
    auto evalWidth(float _key) const -> float;
    auto evalColor(float _key) const -> uint32_t;
    auto nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator;
};

}
