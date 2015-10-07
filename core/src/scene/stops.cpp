#include "stops.h"
#include "scene/styleParam.h"
#include "platform.h"

#include "csscolorparser.hpp"
#include "yaml-cpp/yaml.h"
#include <algorithm>

namespace Tangram {

auto Stops::Colors(const YAML::Node& _node) -> Stops {
    Stops stops;
    if (!_node.IsSequence()) { return stops; }

    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();

        // parse color from sequence or string
        Color color;
        YAML::Node colorNode = frameNode[1];
        if (colorNode.IsScalar()) {
            color.abgr = CSSColorParser::parse(colorNode.as<std::string>()).getInt();
        } else if (colorNode.IsSequence() && colorNode.size() >= 3) {
            color.r = colorNode[0].as<float>() * 255.;
            color.g = colorNode[1].as<float>() * 255.;
            color.b = colorNode[2].as<float>() * 255.;
            float alpha = colorNode.size() > 3 ? colorNode[3].as<float>() : 1.f;
            color.a = alpha * 255.;
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

auto Stops::Widths(const YAML::Node& _node, const MapProjection& _projection) -> Stops {
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

    double range = exp2(upper->key - lower->key) - 1.0;
    double pos = exp2(_key - lower->key) - 1.0;

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
    if (upper == frames.end())  { return lower->color.abgr; }
    if (lower < frames.begin()) { return upper->color.abgr; }

    float lerp = (_key - lower->key) / (upper->key - lower->key);

    return Color::mix(lower->color, upper->color, lerp).abgr;

}

auto Stops::nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator {

    return std::lower_bound(frames.begin(), frames.end(), _key,
                            [](const Frame& f, float z) { return f.key < z; });

}

}
