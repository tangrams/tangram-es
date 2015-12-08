#pragma once

#include "util/color.h"
#include "scene/styleParam.h"
#include "variant.hpp"

#include <vector>

namespace YAML {
    class Node;
}

namespace Tangram {

class MapProjection;

using StopValue = variant<none_type, float, Color, glm::vec2>;

struct Stops {

    struct Frame {
        float key = 0;
        StopValue value;
        Frame(float _k, float _v) : key(_k), value(_v) {}
        Frame(float _k, Color _c) : key(_k), value(_c) {}
    };

    std::vector<Frame> frames;
    static Stops Colors(const YAML::Node& _node);
    static Stops Widths(const YAML::Node& _node, const MapProjection& _projection, const std::vector<Unit>& _units);
    static Stops FontSize(const YAML::Node& _node);

    Stops(const std::vector<Frame>& _frames) : frames(_frames) {}
    Stops() {}

    auto evalFloat(float _key) const -> float;
    auto evalWidth(float _key) const -> float;
    auto evalColor(float _key) const -> uint32_t;
    auto evalVec(float _key) const -> glm::vec2;
    auto nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator;
};

}
