#include "stops.h"
#include "scene/styleParam.h"
#include "platform.h"

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

double widthMeterToPixel(float _zoom, double _tileSize, double _width) {
    // pixel per meter at z == 0
    double meterRes = _tileSize / (2.0 * MapProjection::HALF_CIRCUMFERENCE);
    // pixel per meter at zoom
    meterRes *= exp2(_zoom);

    return _width * meterRes;
}

auto Stops::Width(const YAML::Node& _node, const MapProjection& _projection) -> Stops {
    Stops stops;
    if (!_node.IsSequence()) { return stops; }

    double tileSize = _projection.TileSize();

    bool lastIsMeter;
    float lastKey = 0;
    float lastMeter = 0;

    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();

        if (lastKey > key) {
            logMsg("Invalid stop order: key %f > %f\n", lastKey, key);
            continue;
        }
        lastKey = key;

        StyleParam::ValueUnitPair width;
        width.unit = Unit::meter;
        size_t start = 0;

        if (StyleParam::parseValueUnitPair(frameNode[1].Scalar(), start, width)){
            if (width.unit == Unit::meter) {
                float w = widthMeterToPixel(key, tileSize, width.value);
                stops.frames.emplace_back(key, w);

                lastIsMeter = true;
                lastMeter = width.value;

                logMsg("Insert stop %d: %f => %f\n", int(key), w, width.value);

            } else {
                stops.frames.emplace_back(key, width.value);
                lastIsMeter = false;

            }
        } else {
            logMsg("could not parse node %s\n", Dump(frameNode[1]).c_str());
        }
    }
    // Append stop at max-zoom to continue scaling after the last stop
    // TODO: define MAX_ZOOM == 24
    if (lastIsMeter && lastKey < 24) {
        float w = widthMeterToPixel(24, tileSize, lastMeter);
        logMsg("Insert last stop  %f\n", w);
        stops.frames.emplace_back(24, w);
    }

    return stops;
}

auto Stops::evalWidth(float _key) const -> float {

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;

    if (upper == frames.end())  { return lower->value; }
    if (lower < frames.begin()) { return upper->value; }

    if (upper->key <= _key) { return upper->value; }
    if (lower->key >= _key) { return lower->value; }

    // double range1 = exp2(upper->key) / exp2(lower->key) - 1.0;
    // double pos1 = exp2(_key) / exp2(lower->key) - 1.0;

    double range = exp2(upper->key - lower->key) - 1.0;
    double pos = exp2(_key - lower->key) - 1.0;
    //logMsg("p: %f / %f r:%f / %f\n", pos1, pos, range1, range);

    double lerp = pos / range;

    return lower->value * (1 - lerp) + upper->value * lerp;

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
