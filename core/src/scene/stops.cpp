#include "stops.h"
#include "csscolorparser.hpp"
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
            uint32_t color = 0;
            YAML::Node colorNode = frameNode[1];
            if (colorNode.IsScalar()) {
                color = CSSColorParser::parse(colorNode.as<std::string>()).getInt();
            } else if (colorNode.IsSequence()) {
                uint32_t r, g, b, a;
                r = CLAMP(colorNode[0].as<float>() * 255., 0., 255.);
                g = CLAMP(colorNode[1].as<float>() * 255., 0., 255.);
                b = CLAMP(colorNode[2].as<float>() * 255., 0., 255.);
                a = CLAMP(colorNode[3].as<float>() * 255., 0., 255.);
                color = (a << 24) + (b << 16) + (g << 8) + (r);
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
    return (upper == frames.end()) ? (upper - 1)->color : upper->color;

    // Colors aren't interpolated. Linear interpolation in RGBA is usually not what you want,
    // but I need to check on how tangram-js handles this. 

}

auto Stops::nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator {

    return std::lower_bound(frames.begin(), frames.end(), _key,
                            [](const Frame& f, float z) { return f.key < z; });
    
}

}