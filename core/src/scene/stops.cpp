#include "stops.h"
#include "scene/styleParam.h"

#include "csscolorparser.hpp"
#include "yaml-cpp/yaml.h"
#include <cassert>
#include <algorithm>

#define RED(col) (col % 256)
#define GRE(col) ((col >> 8) % 256)
#define BLU(col) ((col >> 16) % 256)
#define ALP(col) ((col >> 24) % 256)

namespace Tangram {

auto Stops::Color(const YAML::Node& _node) -> Stops {
    Stops stops;
    if (!_node.IsSequence()) { return stops; }

    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();

        // parse color from sequence or string
        uint32_t color = 0;
        YAML::Node colorNode = frameNode[1];
        if (colorNode.IsScalar()) {
            color = CSSColorParser::parse(colorNode.as<std::string>()).getInt();
        } else if (colorNode.IsSequence() && colorNode.size() >= 3) {
            float alpha = colorNode.size() > 3 ? colorNode[3].as<float>() : 1.f;
            color = alpha * 255.;
            color = (color << 8) + colorNode[2].as<float>() * 255.;
            color = (color << 8) + colorNode[1].as<float>() * 255.;
            color = (color << 8) + colorNode[0].as<float>() * 255.;
        }
        stops.frames.emplace_back(key, color);
    }
    return stops;
}

double meterToPixel(int _zoom, double _tileSize) {
    double meterRes = 2.0 * MapProjection::HALF_CIRCUMFERENCE / _tileSize;
    double invRes = (1 << _zoom) / meterRes;
    return invRes;
}

auto Stops::Width(const YAML::Node& _node, const MapProjection& _projection) -> Stops {
    Stops stops;
    if (!_node.IsSequence()) { return stops; }

    double tileSize = _projection.TileSize();

    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();

        StyleParam::ValueUnitPair width;
        width.unit = Unit::meter;
        size_t start = 0;

        if (StyleParam::parseValueUnitPair(frameNode[1].Scalar(), start, width)){
            if (width.unit == Unit::meter) {
                // TODO: can a key be something different than integer?
                stops.frames.emplace_back(key, float(width.value * meterToPixel(key, tileSize)));
            } else {
                stops.frames.emplace_back(key, width.value);
            }
        }
    }
    return stops;
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
    if (upper == frames.end())  { return lower->color; }
    if (lower < frames.begin()) { return upper->color; }

    float lerp = (_key - lower->key) / (upper->key - lower->key);
    auto lowerColor = lower->color;
    auto upperColor = upper->color;

    uint32_t color = 0;
    color = (color << 8) + ALP(lowerColor) * (1. - lerp) + ALP(upperColor) * lerp;
    color = (color << 8) + BLU(lowerColor) * (1. - lerp) + BLU(upperColor) * lerp;
    color = (color << 8) + GRE(lowerColor) * (1. - lerp) + GRE(upperColor) * lerp;
    color = (color << 8) + RED(lowerColor) * (1. - lerp) + RED(upperColor) * lerp;
    return color;

}

auto Stops::nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator {

    return std::lower_bound(frames.begin(), frames.end(), _key,
                            [](const Frame& f, float z) { return f.key < z; });

}

}
