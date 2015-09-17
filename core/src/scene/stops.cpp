#include "stops.h"
#include "util/geom.h"
#include "yaml-cpp/yaml.h"
#include <cassert>
#include <algorithm>

namespace Tangram {

Stops::Stops(const YAML::Node& _node, bool _isColor) {

    if (!_node.IsSequence()) { return; }

    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();
        if (_isColor) {
            // parse color from sequence or string
            Color color;
            YAML::Node colorNode = frameNode[1];
            if (colorNode.IsScalar()) {
                color = CSSColorParser::parse(colorNode.as<std::string>());
            } else if (colorNode.IsSequence() && colorNode.size() >= 3) {
                float alpha = colorNode.size() > 3 ? colorNode[3].as<float>() : 1.f;
                color = Color(
                    colorNode[0].as<float>() * 255,
                    colorNode[1].as<float>() * 255,
                    colorNode[2].as<float>() * 255,
                    alpha
                );
            }
            frames.emplace_back(key, color);
        } else {
            // parse distance from string
            float value = frameNode[1].as<float>();
            frames.emplace_back(key, value);
        }
    }
}

auto Stops::evalFloat(float _key) const -> float {

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;

    if (upper == frames.end()) { return lower->value; }
    if (lower < frames.begin()) { return upper->value; }

    float lerp = (_key - lower->key) / (upper->key - lower->key);

    return (lower->value * (1 - lerp) + upper->value * lerp);

}

auto Stops::evalColor(float _key) const -> uint32_t {

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;
    if (upper == frames.end()) { return lower->color.getInt(); }
    if (lower < frames.begin()) { return upper->color.getInt(); }

    float lerp = (_key - lower->key) / (upper->key - lower->key);
    auto lowerColor = lower->color;
    auto upperColor = upper->color;

    return Color(
        (1. - lerp) * lowerColor.r + lerp * upperColor.r,
        (1. - lerp) * lowerColor.g + lerp * upperColor.g,
        (1. - lerp) * lowerColor.b + lerp * upperColor.b,
        (1. - lerp) * lowerColor.a + lerp * upperColor.a
    ).getInt();

}

auto Stops::nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator {

    return std::lower_bound(frames.begin(), frames.end(), _key,
                            [](const Frame& f, float z) { return f.key < z; });
    
}

}