#pragma once

#include "scene/styleParam.h"
#include "util/color.h"
#include "variant.hpp"

#include <vector>

namespace YAML {
    class Node;
}

namespace Tangram {

class MapProjection;
struct SpriteNode;

using StopValue = variant<none_type, float, Color, glm::vec2, StyleParam::SizeValue>;

struct Stops {

    struct Frame {
        float key = 0;
        StopValue value;
        Frame(float _k, float _v) : key(_k), value(_v) {}
        Frame(float _k, Color _c) : key(_k), value(_c) {}
        Frame(float _k, glm::vec2 _v) : key(_k), value(_v) {}
        Frame(float _k, StyleParam::SizeValue sizeValue) : key(_k), value(sizeValue) {}
    };

    std::vector<Frame> frames;
    static Stops Colors(const YAML::Node& _node);
    static Stops Widths(const YAML::Node& _node, const MapProjection& _projection, uint8_t _units);
    static Stops FontSize(const YAML::Node& _node);
    static Stops Sizes(const YAML::Node& _node, uint8_t _units);
    static Stops Offsets(const YAML::Node& _node, uint8_t _units);
    static Stops Numbers(const YAML::Node& node);

    Stops(const std::vector<Frame>& _frames) : frames(_frames) {}
    Stops(const Stops& rhs) = default;
    Stops() {}

    auto evalFloat(float _key) const -> float;
    auto evalExpFloat(float _key) const -> float;
    auto evalColor(float _key) const -> uint32_t;
    auto evalVec2(float _key) const -> glm::vec2;
    auto evalExpVec2(float _key) const -> glm::vec2;
    auto evalSize(float _key, const glm::vec2& cssSize) const -> glm::vec2;
    auto nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator;

    static void eval(const Stops& _stops, StyleParamKey _key, float _zoom, StyleParam::Value& _result);
};

}
