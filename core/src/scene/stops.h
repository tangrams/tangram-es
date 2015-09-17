#pragma once

#include "csscolorparser.hpp"
#include <vector>

namespace YAML {
    class Node;
}

namespace Tangram {

using Color = CSSColorParser::Color;

struct Frame {
    float key = 0;
    union {
        float value;
        Color color;
    };
    Frame(float _k, float _v) : key(_k), value(_v) {}
    Frame(float _k, Color _c) : key(_k), color(_c) {}
};

struct Stops {

    std::vector<Frame> frames;

    Stops(const YAML::Node& _node, bool _isColor = false);
    Stops(const std::vector<Frame>& _frames) : frames(_frames) {}

    auto evalFloat(float _key) const -> float;
    auto evalColor(float _key) const -> uint32_t;
    auto nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator;
};

}
