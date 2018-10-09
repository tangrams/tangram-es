#include "scene/stops.h"

#include "platform.h"
#include "log.h"
#include "scene/styleParam.h"
#include "scene/spriteAtlas.h"
#include "util/mapProjection.h"

#include <algorithm>
#include "csscolorparser.hpp"
#include "yaml-cpp/yaml.h"

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
    double meterRes = _tileSize / MapProjection::EARTH_CIRCUMFERENCE_METERS;
    // pixel per meter at zoom
    meterRes *= exp2(_zoom);

    return _width * meterRes;
}

auto Stops::FontSize(const YAML::Node& _node) -> Stops {
    Stops stops;

    if (!_node.IsSequence()) {
        return stops;
    }

    float lastKey = 0;
    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();

        if (lastKey > key) {
            LOGW("Invalid stop order: key %f > %f", lastKey, key);
            continue;
        }

        lastKey = key;
        float pixelSize;

        if (StyleParam::parseFontSize(frameNode[1].Scalar(), pixelSize)) {
            stops.frames.emplace_back(key, pixelSize);
        } else {
            LOGW("Error while parsing font size stops: %f %s", key, Dump(frameNode[1]).c_str());
        }
    }

    return stops;
}

auto Stops::Sizes(const YAML::Node& _node, uint8_t _units) -> Stops {
    Stops stops;

    // mixed dim stops not allowed for sizes (except when 1D stop uses %)
    bool has1DSize = false;
    bool has2DSize = false;

    if (!_node.IsSequence()) {
        return stops;
    }

    float lastKey = 0;

    auto constructFrame = [&](const auto& _frameNode, StyleParam::ValueUnitPair& _result) -> bool {
        if (StyleParam::parseSizeUnitPair(_frameNode.Scalar(), 0, _result)) {
            if ( !(_units & _result.unit) ) {
                LOGW("Size StyleParam can only take in pixel, %% or auto values in: %s", Dump(_node).c_str());
                return false;
            }
        } else {
            LOGW("could not parse node %s\n", Dump(_frameNode).c_str());
            return false;
        }
        return true;
    };

    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();

        if (lastKey > key) {
            LOGW("Invalid stop order: key %f > %f", lastKey, key);
            continue;
        }
        lastKey = key;

        if (frameNode[1].IsScalar()) {
            StyleParam::SizeValue sizeValue;
            if (!constructFrame(frameNode[1], sizeValue.x)) {
                continue;
            }
            if (!sizeValue.x.isPercentage()) {
                has1DSize = true;
            }
            stops.frames.emplace_back(key, sizeValue);
        } else if (frameNode[1].IsSequence()) {
            StyleParam::SizeValue sizeValue;
            const auto& sequenceNode = frameNode[1];
            if (!constructFrame(sequenceNode[0], sizeValue.x) ||
                !constructFrame(sequenceNode[1], sizeValue.y)) {
                continue;
            }
            has2DSize = true;
            stops.frames.emplace_back(key, sizeValue);
        }
        if (has1DSize && has2DSize) {
            LOGW("Cannot have mixed dimensions stops for Size style parameter: %s", Dump(_node).c_str());
            stops.frames.clear();
            return stops;
        }
    }
    return stops;
}

auto Stops::Offsets(const YAML::Node& _node, uint8_t _units) -> Stops {
    Stops stops;
    if (!_node.IsSequence()) {
        return stops;
    }
    float lastKey = 0;
    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();

        if (lastKey > key) {
            LOGW("Invalid stop order: key %f > %f", lastKey, key);
            continue;
        }
        lastKey = key;

        if (frameNode[1].IsSequence()) {
            std::vector<StyleParam::ValueUnitPair> widths;
            Unit lastUnit = Unit::pixel;

            for (const auto& sequenceNode : frameNode[1]) {
                StyleParam::ValueUnitPair width;
                width.unit = Unit::pixel; // default to pixel
                if (StyleParam::parseValueUnitPair(sequenceNode.Scalar(), 0, width)) {
                    widths.push_back(width);
                    if (lastUnit != width.unit) {
                        LOGW("Mixed units not allowed for stop values", Dump(frameNode[1]).c_str());
                    }
                    lastUnit = width.unit;
                } else {
                    LOGW("could not parse node %s", Dump(sequenceNode).c_str());
                }
            }
            if (widths.size() == 2) {
                if ( !(widths[0].unit & _units ) &&
                        !(widths[1].unit & _units) ) {
                    LOGW("Non-pixel unit not allowed for multidimensionnal stop values");
                }
                stops.frames.emplace_back(key, glm::vec2(widths[0].value, widths[1].value));
            }
        }
    }

    return stops;
}

auto Stops::Widths(const YAML::Node& _node, uint8_t _units) -> Stops {
    Stops stops;
    if (!_node.IsSequence()) { return stops; }

    double tileSize = MapProjection::tileSize();

    bool lastIsMeter = false;
    float lastKey = 0;
    float lastMeter = 0;

    for (const auto& frameNode : _node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }
        float key = frameNode[0].as<float>();

        if (lastKey > key) {
            LOGW("Invalid stop order: key %f > %f\n", lastKey, key);
            continue;
        }
        lastKey = key;

        StyleParam::ValueUnitPair width;
        width.unit = Unit::meter;
        size_t start = 0;

        if (StyleParam::parseValueUnitPair(frameNode[1].Scalar(), start, width)) {

            if (! (width.unit & _units) ) {
                LOGW("Invalid unit is being used for stop %s", Dump(frameNode[1]).c_str());
            }

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
            LOGW("could not parse node %s\n", Dump(frameNode[1]).c_str());
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

auto Stops::Numbers(const YAML::Node& node) -> Stops {
    Stops stops;
    if (!node.IsSequence()) { return stops; }

    float lastKey = 0;

    for (const auto frameNode : node) {
        if (!frameNode.IsSequence() || frameNode.size() != 2) { continue; }

        float key = frameNode[0].as<float>();
        if (lastKey > key) {
            LOGW("Invalid stop order: key %f > %f\n", lastKey, key);
            continue;
        }
        lastKey = key;

        float value = frameNode[1].as<float>();
        stops.frames.emplace_back(key, value);
    }

    return stops;
}

auto Stops::evalExpFloat(float _key) const -> float {
    if (frames.empty()) { return 0; }

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;

    if (upper == frames.end())  {
        return lower->value.get<float>();
    }
    if (lower < frames.begin()) {
        return upper->value.get<float>();
    }

    if (upper->key <= _key) {
        return upper->value.get<float>();
    }
    if (lower->key >= _key) {
        return lower->value.get<float>();
    }

    double range = exp2(upper->key - lower->key) - 1.0;
    double pos = exp2(_key - lower->key) - 1.0;

    double lerp = pos / range;

    return lower->value.get<float>() * (1 - lerp) + upper->value.get<float>() * lerp;
}

auto Stops::evalFloat(float _key) const -> float {
    if (frames.empty()) { return 0; }

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;

    if (upper == frames.end()) {
        return lower->value.get<float>();
    }
    if (lower < frames.begin()) {
        return upper->value.get<float>();
    }

    float lerp = (_key - lower->key) / (upper->key - lower->key);

    return (lower->value.get<float>() * (1 - lerp) + upper->value.get<float>() * lerp);
}

auto Stops::evalColor(float _key) const -> uint32_t {
    if (frames.empty()) { return 0; }

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;
    if (upper == frames.end())  {
        return lower->value.get<Color>().abgr;
    }
    if (lower < frames.begin()) {
        return upper->value.get<Color>().abgr;
    }

    float lerp = (_key - lower->key) / (upper->key - lower->key);

    return Color::mix(lower->value.get<Color>(), upper->value.get<Color>(), lerp).abgr;
}

auto Stops::evalExpVec2(float _key) const -> glm::vec2 {
    if (frames.empty()) { return glm::vec2{0.f}; }

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;

    if (upper == frames.end()) {
        return lower->value.get<glm::vec2>();
    }
    if (lower < frames.begin()) {
        return upper->value.get<glm::vec2>();
    }

    double range = exp2(upper->key - lower->key) - 1.0;
    double pos = exp2(_key - lower->key) - 1.0;

    double lerp = pos / range;

    const glm::vec2& lowerVal = lower->value.get<glm::vec2>();
    const glm::vec2& upperVal = upper->value.get<glm::vec2>();

    return glm::vec2(lowerVal.x * (1 - lerp) + upperVal.x * lerp,
                     lowerVal.y * (1 - lerp) + upperVal.y * lerp);

}

auto Stops::evalVec2(float _key) const -> glm::vec2 {
    if (frames.empty()) { return glm::vec2{0.f}; }

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;

    if (upper == frames.end()) {
        return lower->value.get<glm::vec2>();
    }
    if (lower < frames.begin()) {
        return upper->value.get<glm::vec2>();
    }

    float lerp = (_key - lower->key) / (upper->key - lower->key);

    const glm::vec2& lowerVal = lower->value.get<glm::vec2>();
    const glm::vec2& upperVal = upper->value.get<glm::vec2>();

    return glm::vec2(lowerVal.x * (1 - lerp) + upperVal.x * lerp,
                     lowerVal.y * (1 - lerp) + upperVal.y * lerp);

}

auto Stops::evalSize(float _key, const glm::vec2& _cssSize) const -> glm::vec2 {

    if (frames.empty()) { return {NAN, NAN}; }

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;

    if (upper == frames.end()) {
        return lower->value.get<StyleParam::SizeValue>().getSizePixels(_cssSize);
    }
    if (lower < frames.begin()) {
        return upper->value.get<StyleParam::SizeValue>().getSizePixels(_cssSize);
    }

    double range = exp2(upper->key - lower->key) - 1.0;
    double pos = exp2(_key - lower->key) - 1.0;

    double lerp = pos / range;

    const glm::vec2 lowerVal = lower->value.get<StyleParam::SizeValue>().getSizePixels(_cssSize);
    const glm::vec2 upperVal = upper->value.get<StyleParam::SizeValue>().getSizePixels(_cssSize);

    return glm::vec2(lowerVal.x * (1 - lerp) + upperVal.x * lerp,
                     lowerVal.y * (1 - lerp) + upperVal.y * lerp);

}

auto Stops::nearestHigherFrame(float _key) const -> std::vector<Frame>::const_iterator {

    return std::lower_bound(frames.begin(), frames.end(), _key,
                            [](const Frame& f, float z) { return f.key < z; });
}

void Stops::eval(const Stops& _stops, StyleParamKey _key, float _zoom, StyleParam::Value& _result) {

    /* StyleParam::size stops can not have a generic evaluation, and
     * requires more context and is handled in the pointStyleBuilder
     */
    if (StyleParam::isSize(_key)) { return; }

    if (StyleParam::isColor(_key)) {
        _result = _stops.evalColor(_zoom);
    } else if (StyleParam::isWidth(_key)) {
        _result = _stops.evalExpFloat(_zoom);
    } else if (StyleParam::isOffsets(_key)) {
        _result = _stops.evalVec2(_zoom);
    } else {
        _result = _stops.evalFloat(_zoom);
    }
}

}
