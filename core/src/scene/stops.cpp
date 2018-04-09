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
    double meterRes = _tileSize / (2.0 * MapProjection::HALF_CIRCUMFERENCE);
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

auto Stops::Sizes(const YAML::Node& _node, const std::vector<Unit>& _units) -> Stops {
    Stops stops;

    // mixed dim stops not allowed for sizes
    bool hasSingleDim = true;

    if (!_node.IsSequence()) {
        return stops;
    }

    float lastKey = 0;

    auto constructFrame = [&](const auto& _frameNode, size_t offset, StyleParam::ValueUnitPair& _result) -> bool {
        if (StyleParam::parseSizeUnitPair(_frameNode.Scalar(), offset, _result)) {
            if (std::find(_units.begin(), _units.end(), _result.unit) == _units.end()) {
                LOGW("Size StyleParam can only take in pixel, % or auto values in: %s", Dump(_node).c_str());
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

            // Check for mixed dimension stops
            if (stops.frames.empty()) {
                hasSingleDim = true;
            } else if (!hasSingleDim) {
                LOGW("Can not have mixed dimensions stops for Size style parameter.");
                stops.frames.clear();
                return stops;
            }

            StyleParam::SizeValue sizeValue;
            // default size values
            sizeValue.fill({NAN, Unit::pixel});
            if (constructFrame(frameNode[1], 0, sizeValue[0])) {
                stops.frames.emplace_back(key, sizeValue);
            }
        } else if (frameNode[1].IsSequence()) {

            // Check for mixed dimension stops
            if (stops.frames.empty()) {
                hasSingleDim = false;
            } else if (hasSingleDim) {
                LOGW("Can not have mixed dimensions stops for Size style parameter.");
                stops.frames.clear();
                return stops;
            }
            StyleParam::SizeValue sizeValue;
            // default size values
            sizeValue.fill({NAN, Unit::pixel});
            size_t index = 0;
            for (const auto& sequenceNode : frameNode[1]) {
                if (!constructFrame(sequenceNode, 0, sizeValue[index])) {
                    break;
                }
                index++;
            }
            if (index == 2) {
                stops.frames.emplace_back(key, sizeValue);
            }
        }
    }
    return stops;
}

auto Stops::Offsets(const YAML::Node& _node, const std::vector<Unit>& _units) -> Stops {
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
                if (widths[0].unit != Unit::pixel || widths[1].unit != Unit::pixel) {
                    LOGW("Non-pixel unit not allowed for multidimensionnal stop values");
                }
                stops.frames.emplace_back(key, glm::vec2(widths[0].value, widths[1].value));
            }
        }
    }

    return stops;
}

auto Stops::Widths(const YAML::Node& _node, const MapProjection& _projection, const std::vector<Unit>& _units) -> Stops {
    Stops stops;
    if (!_node.IsSequence()) { return stops; }

    double tileSize = _projection.TileSize();

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
            bool valid = false;
            for (auto& unit : _units) {
                if (width.unit == unit) {
                    valid = true;
                    break;
                }
            }

            if (!valid) {
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

auto Stops::evalSize(float _key, const glm::vec2& _cssSize, float _aspectRatio, bool _useCssSize) const -> glm::vec2 {

    if (frames.empty()) { return {NAN, NAN}; }

    auto getSizeValue = [&](StyleParam::SizeValue size) -> glm::vec2 {
        if ( (!_useCssSize && (!size[0].isPixel() && !size[1].isPixel()) ) ||
             (size[0].isAuto() && size[1].isAuto()) ) {
            return {NAN, NAN};
        }
        if (size[0].isPixel() && size[1].isPixel()) {
            if (std::isnan(size[0].value) && std::isnan(size[1].value)) {
                return _cssSize;
            } else if (size[0].value == 0.f || std::isnan(size[1].value)) {
                return glm::vec2(size[0].value);
            } else {
                return glm::vec2(size[0].value, size[1].value);
            }
        } else {
            if (size[0].isPercentage()) {
                return _cssSize * (size[0].value * 0.01f);
            } else if (size[0].isAuto()) {
                return glm::vec2(size[1].value * _aspectRatio, size[1].value);
            } else {
                return glm::vec2(size[0].value, size[0].value / _aspectRatio);
            }
        }
    };

    auto upper = nearestHigherFrame(_key);
    auto lower = upper - 1;

    if (upper == frames.end()) {
        return getSizeValue(lower->value.get<StyleParam::SizeValue>());
    }
    if (lower < frames.begin()) {
        return getSizeValue(upper->value.get<StyleParam::SizeValue>());
    }

    double range = exp2(upper->key - lower->key) - 1.0;
    double pos = exp2(_key - lower->key) - 1.0;

    double lerp = pos / range;

    const glm::vec2& lowerVal = getSizeValue(lower->value.get<StyleParam::SizeValue>());
    const glm::vec2& upperVal = getSizeValue(upper->value.get<StyleParam::SizeValue>());

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
