#pragma once

#include "util/color.h"
#include "util/variant.h"

#include "glm/vec2.hpp"

#include <vector>

namespace YAML {
    class Node;
}

namespace Tangram {

enum class Unit : uint8_t;

using StopValue = variant<none_type, float, Color, glm::vec2>;

struct Stops {

    struct Frame {
        float key = 0;
        StopValue value;
        Frame(float _k, float _v) : key(_k), value(_v) {}
        Frame(float _k, Color _c) : key(_k), value(_c) {}
        Frame(float _k, glm::vec2 _v) : key(_k), value(_v) {}

        bool operator==(const Frame& _other) const {
            return key == _other.key && value == _other.value;
        }

    };

    std::vector<Frame> frames;
    static Stops Colors(const YAML::Node& _node);
    static Stops Widths(const YAML::Node& _node, double _tileSize, const std::vector<Unit>& _units);
    static Stops FontSize(const YAML::Node& _node);
    static Stops Offsets(const YAML::Node& _node, const std::vector<Unit>& _units);
    static Stops Numbers(const YAML::Node& node);

    Stops(const std::vector<Frame>& _frames) : frames(_frames) {}
    Stops(const Stops& rhs) = default;
    Stops() {}

    auto evalFloat(float _key) const -> float;
    auto evalWidth(float _key) const -> float;
    auto evalColor(float _key) const -> uint32_t;
    auto evalVec2(float _key) const -> glm::vec2;
    auto nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator;

    bool operator==(const Stops& _other) const {
        if (frames.size() != _other.frames.size()) { return false; }
        for (size_t i = 0; i < frames.size(); i++) {
            if (!(frames[i] == _other.frames[i])) { return false;}
        }
        return true;
    }

};

}
