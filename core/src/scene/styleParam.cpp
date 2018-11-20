#include "scene/styleParam.h"

#include "log.h"
#include "platform.h"
#include "style/textStyle.h"
#include "util/builders.h" // for cap, join
#include "util/color.h"
#include "util/extrude.h"
#include "util/floatFormatter.h"
#include "util/geom.h" // for CLAMP
#include "util/mapProjection.h"
#include "util/yamlUtil.h"

#include "csscolorparser.hpp"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/convert.h"
#include "yaml-cpp/node/emit.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <style/pointStyle.h>

namespace Tangram {

const std::map<std::string, StyleParamKey> s_StyleParamMap = {
    {"align", StyleParamKey::text_align},
    {"anchor", StyleParamKey::anchor},
    {"angle", StyleParamKey::angle},
    {"buffer", StyleParamKey::buffer},
    {"cap", StyleParamKey::cap},
    {"collide", StyleParamKey::collide},
    {"color", StyleParamKey::color},
    {"extrude", StyleParamKey::extrude},
    {"flat", StyleParamKey::flat},
    {"font:family", StyleParamKey::text_font_family},
    {"font:fill", StyleParamKey::text_font_fill},
    {"font:size", StyleParamKey::text_font_size},
    {"font:stroke:color", StyleParamKey::text_font_stroke_color},
    {"font:stroke:width", StyleParamKey::text_font_stroke_width},
    {"font:style", StyleParamKey::text_font_style},
    {"font:transform", StyleParamKey::text_transform},
    {"font:weight", StyleParamKey::text_font_weight},
    {"interactive", StyleParamKey::interactive},
    {"join", StyleParamKey::join},
    {"max_lines", StyleParamKey::text_max_lines},
    {"miter_limit", StyleParamKey::miter_limit},
    {"none", StyleParamKey::none},
    {"offset", StyleParamKey::offset},
    {"order", StyleParamKey::order},
    {"outline:cap", StyleParamKey::outline_cap},
    {"outline:color", StyleParamKey::outline_color},
    {"outline:join", StyleParamKey::outline_join},
    {"outline:miter_limit", StyleParamKey::outline_miter_limit},
    {"outline:order", StyleParamKey::outline_order},
    {"outline:style", StyleParamKey::outline_style},
    {"outline:visible", StyleParamKey::outline_visible},
    {"outline:width", StyleParamKey::outline_width},
    {"placement", StyleParamKey::placement},
    {"placement_min_length_ratio", StyleParamKey::placement_min_length_ratio},
    {"placement_spacing", StyleParamKey::placement_spacing},
    {"priority", StyleParamKey::priority},
    {"repeat_distance", StyleParamKey::repeat_distance},
    {"repeat_group", StyleParamKey::repeat_group},
    {"size", StyleParamKey::size},
    {"sprite", StyleParamKey::sprite},
    {"sprite_default", StyleParamKey::sprite_default},
    {"style", StyleParamKey::style},
    {"text:align", StyleParamKey::text_align},
    {"text:anchor", StyleParamKey::text_anchor},
    {"text:buffer", StyleParamKey::text_buffer},
    {"text:collide", StyleParamKey::text_collide},
    {"text:font:family", StyleParamKey::text_font_family},
    {"text:font:fill", StyleParamKey::text_font_fill},
    {"text:font:size", StyleParamKey::text_font_size},
    {"text:font:stroke:color", StyleParamKey::text_font_stroke_color},
    {"text:font:stroke:width", StyleParamKey::text_font_stroke_width},
    {"text:font:style", StyleParamKey::text_font_style},
    {"text:font:transform", StyleParamKey::text_transform},
    {"text:font:weight", StyleParamKey::text_font_weight},
    {"text:interactive", StyleParamKey::text_interactive},
    {"text:max_lines", StyleParamKey::text_max_lines},
    {"text:offset", StyleParamKey::text_offset},
    {"text:optional", StyleParamKey::text_optional},
    {"text:order", StyleParamKey::text_order},
    {"text:priority", StyleParamKey::text_priority},
    {"text:repeat_distance", StyleParamKey::text_repeat_distance},
    {"text:repeat_group", StyleParamKey::text_repeat_group},
    {"text:text_source", StyleParamKey::text_source},
    {"text:text_source:left", StyleParamKey::text_source_left},
    {"text:text_source:right", StyleParamKey::text_source_right},
    {"text:text_wrap", StyleParamKey::text_wrap},
    {"text:transition:hide:time", StyleParamKey::text_transition_hide_time},
    {"text:transition:selected:time", StyleParamKey::text_transition_selected_time},
    {"text:transition:show:time", StyleParamKey::text_transition_show_time},
    {"text:visible", StyleParamKey::text_visible},
    {"text_source", StyleParamKey::text_source},
    {"text_source:left", StyleParamKey::text_source_left},
    {"text_source:right", StyleParamKey::text_source_right},
    {"text_wrap", StyleParamKey::text_wrap},
    {"texture", StyleParamKey::texture},
    {"tile_edges", StyleParamKey::tile_edges},
    {"transition:hide:time", StyleParamKey::transition_hide_time},
    {"transition:selected:time", StyleParamKey::transition_selected_time},
    {"transition:show:time", StyleParamKey::transition_show_time},
    {"visible", StyleParamKey::visible},
    {"width", StyleParamKey::width},
};

const std::string& StyleParam::keyName(StyleParamKey _key) {
    static std::string fallback = "bug";
    for (const auto& entry : s_StyleParamMap) {
        if (entry.second == _key) { return entry.first; }
    }
    return fallback;
}

StyleParamKey StyleParam::getKey(const std::string& _key) {
    auto it = s_StyleParamMap.find(_key);
    if (it == s_StyleParamMap.end()) {
        return StyleParamKey::none;
    }
    return it->second;
}

StyleParam::StyleParam(const std::string& _key, const YAML::Node& _value) {
    key = getKey(_key);
    value = none_type{};

    if (key == StyleParamKey::none) {
        LOGW("Unknown StyleParam %s:%s", _key.c_str(), Dump(_value).c_str());
        return;
    }
    value = parseNode(key, _value);
}

StyleParam::StyleParam(StyleParamKey _key, const YAML::Node& _value) {
    key = _key;
    value = parseNode(key, _value);
}


StyleParam::Value StyleParam::parseNode(StyleParamKey key, const YAML::Node& node) {
    UnitSet allowedUnits = unitSetForStyleParam(key);

    switch (key) {
    case StyleParamKey::extrude: {
        return parseExtrudeNode(node);
    }
    case StyleParamKey::text_wrap: {
        int wrapLength = TextStyle::DEFAULT_TEXT_WRAP_LENGTH;
        if (YamlUtil::getBoolOrDefault(node, false)) {
            return (uint32_t)wrapLength;
        } else if (YamlUtil::getInt(node, wrapLength) && wrapLength > 0) {
            return (uint32_t)wrapLength;
        }
        return std::numeric_limits<uint32_t>::max();
    }
    case StyleParamKey::text_offset:
    case StyleParamKey::offset:
    case StyleParamKey::text_buffer:
    case StyleParamKey::buffer: {
        ValueUnitPair scalar;
        if (node.IsScalar() && (parseValueUnitPair(node.Scalar(), scalar) && allowedUnits.contains(scalar.unit))) {
            return glm::vec2(scalar.value);
        }
        UnitVec<glm::vec2> vec;
        if (parseVec2(node, allowedUnits, vec)) {
            return vec.value;
        }
        LOGW("Invalid buffer/offset parameter '%s'.", Dump(node).c_str());
        return Value();
    }
    case StyleParamKey::size: {
        SizeValue vec;
        if (!parseSize(node, allowedUnits, vec)) {
            LOGW("Invalid size parameter '%s'.", Dump(node).c_str());
        }
        return vec;
    }
    case StyleParamKey::transition_hide_time:
    case StyleParamKey::transition_show_time:
    case StyleParamKey::transition_selected_time:
    case StyleParamKey::text_transition_hide_time:
    case StyleParamKey::text_transition_show_time:
    case StyleParamKey::text_transition_selected_time: {
        float time = 0.0f;
        if (!node.IsScalar() || !parseTime(node.Scalar(), time)) {
            LOGW("Invalid time param '%s'", Dump(node).c_str());
        }
        return time;
    }
    case StyleParamKey::text_font_family:
    case StyleParamKey::text_font_weight:
    case StyleParamKey::text_font_style: {
        std::string result;
        if (node.IsScalar()) {
            result = node.Scalar();
            std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        } else {
            LOGW("Invalid font property: %s", Dump(node).c_str());
        }
        return result;
    }
    case StyleParamKey::anchor:
    case StyleParamKey::text_anchor: {
        LabelProperty::Anchors anchorSet;
        LabelProperty::Anchor anchor;
        if (node.IsScalar() && LabelProperty::anchor(node.Scalar(), anchor)) {
            anchorSet.anchor[anchorSet.count++] = anchor;
        } else if (node.IsSequence()) {
            for (const auto& subNode : node) {
                if (anchorSet.count >= LabelProperty::max_anchors) { break; }
                if (subNode.IsScalar() && LabelProperty::anchor(subNode.Scalar(), anchor)) {
                    anchorSet.anchor[anchorSet.count++] = anchor;
                }
            }
        }
        return anchorSet;
    }
    case StyleParamKey::placement: {
        LabelProperty::Placement placement = LabelProperty::Placement::vertex;
        if (!(node.IsScalar() && LabelProperty::placement(node.Scalar(), placement))) {
            LOG("Invalid placement parameter, Setting vertex as default.");
        }
        return placement;
    }
    case StyleParamKey::text_source:
    case StyleParamKey::text_source_left:
    case StyleParamKey::text_source_right: {
        TextSource textSource;
        if (node.IsScalar()) {
            textSource.keys.push_back(node.Scalar());
        } else if (node.IsSequence()) {
            for (const auto& subNode : node) {
                if (subNode.IsScalar()) {
                    textSource.keys.push_back(subNode.Scalar());
                }
            }
        } else {
            LOGW("Invalid text source: %s", Dump(node).c_str());
        }
        return std::move(textSource);
    }
    case StyleParamKey::text_align:
    case StyleParamKey::text_transform:
    case StyleParamKey::sprite:
    case StyleParamKey::sprite_default:
    case StyleParamKey::style:
    case StyleParamKey::outline_style:
    case StyleParamKey::repeat_group:
    case StyleParamKey::text_repeat_group:
    case StyleParamKey::texture: {
        if (node.IsScalar()) {
            return node.Scalar();
        }
        LOGW("Invalid scalar value: %s", Dump(node).c_str());
        return std::string();
    }
    case StyleParamKey::text_font_size: {
        float fontSize = 0.f;
        if (!node.IsScalar() || !parseFontSize(node.Scalar(), fontSize)) {
            LOGW("Invalid font-size '%s'.", node.Scalar().c_str());
        }
        return fontSize;
    }
    case StyleParamKey::flat:
    case StyleParamKey::interactive:
    case StyleParamKey::text_interactive:
    case StyleParamKey::tile_edges:
    case StyleParamKey::visible:
    case StyleParamKey::text_visible:
    case StyleParamKey::outline_visible:
    case StyleParamKey::collide:
    case StyleParamKey::text_optional:
    case StyleParamKey::text_collide: {
        bool result = false;
        if (YamlUtil::getBool(node, result)) {
            return result;
        }
        LOGW("Invalid boolean value %s for key %s", Dump(node).c_str(), StyleParam::keyName(key).c_str());
        break;
    }
    case StyleParamKey::text_order:
        LOGW("text:order parameter is ignored.");
        break;
    case StyleParamKey::order:
    case StyleParamKey::outline_order: {
        int result = -1;
        if (YamlUtil::getInt(node, result)) {
            if (result >= 0) {
                return static_cast<uint32_t>(result);
            }
        } else if (node.IsSequence()) {
            LOGW("NumberProperty '%s' value '%s'", keyName(key).c_str(), Dump(node).c_str());
            if (node.size() == 1 && node[0].IsScalar()) {
                return NumberProperty{node[0].Scalar(),0};
            }
            if (node.size() == 2 && node[0].IsScalar() &&
                YamlUtil::getInt(node[1], result)) {
                return NumberProperty{node[0].Scalar(), result};
            }
        }
        LOGW("Invalid '%s' value '%s'", keyName(key).c_str(), Dump(node).c_str());
        break;
    }
    case StyleParamKey::priority:
    case StyleParamKey::text_max_lines:
    case StyleParamKey::text_priority: {
        int result = -1;
        if (YamlUtil::getInt(node, result)) {
            return static_cast<uint32_t>(result);
        }
        LOGW("Invalid '%s' value '%s'", keyName(key).c_str(), Dump(node).c_str());
        break;
    }
    case StyleParamKey::placement_spacing: {
        ValueUnitPair result;
        if (node.IsScalar() && parseValueUnitPair(node.Scalar(), result)) {
            if (result.unit == Unit::none) {
                result.unit = Unit::pixel;
            }
        }
        if (result.unit != Unit::pixel) {
            LOGW("Invalid placement spacing value '%s'", Dump(node).c_str());
            result.value = PointStyle::DEFAULT_PLACEMENT_SPACING;
            result.unit = Unit::pixel;
        }
        return Width(result);
    }
    case StyleParamKey::repeat_distance:
    case StyleParamKey::text_repeat_distance: {
        ValueUnitPair result;
        if (node.IsScalar() && parseValueUnitPair(node.Scalar(), result)) {
            if (result.unit == Unit::none) {
                result.unit = Unit::pixel;
            }
        }
        if (result.unit != Unit::pixel) {
            LOGW("Invalid repeat distance value '%s'", Dump(node).c_str());
            result.value = MapProjection::tileSize();
            result.unit = Unit::pixel;
        }
        return Width(result);
    }
    case StyleParamKey::width:
    case StyleParamKey::outline_width: {
        ValueUnitPair result;
        if (node.IsScalar() && parseValueUnitPair(node.Scalar(), result)) {
            if (result.unit == Unit::none) {
                result.unit = Unit::meter;
            }
        }
        if (result.unit != Unit::meter && result.unit != Unit::pixel) {
            LOGW("Invalid width value '%s'", Dump(node).c_str());
            result.value = 2.f;
            result.unit = Unit::pixel;
        }
        return Width(result);
    }
    case StyleParamKey::angle: {
        if (node.IsScalar() && node.Scalar() == "auto") {
            return NAN;
        }
        float number;
        if (YAML::convert<float>::decode(node, number)) {
            return number;
        }
        LOGW("Invalid angle value: %s", Dump(node).c_str());
        break;
    }
    case StyleParamKey::miter_limit:
    case StyleParamKey::outline_miter_limit:
    case StyleParamKey::placement_min_length_ratio:
    case StyleParamKey::text_font_stroke_width: {
        float floatValue;
        if (YamlUtil::getFloat(node, floatValue, true)) {
            return floatValue;
        } else {
            LOGW("Invalid width value: %s", Dump(node).c_str());
        }
        break;
    }
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::text_font_fill:
    case StyleParamKey::text_font_stroke_color: {
        Color result;
        if (parseColor(node, result)) {
            return result.abgr;
        }
        LOGW("Invalid color value: %s", Dump(node).c_str());
        break;
    }
    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
        if (node.IsScalar()) {
            return static_cast<uint32_t>(CapTypeFromString(node.Scalar()));
        }
        break;
    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        if (node.IsScalar()) {
            return static_cast<uint32_t>(JoinTypeFromString(node.Scalar()));
        }
        break;
    default:
        break;
    }

    return none_type{};
}

StyleParam::Value StyleParam::parseString(StyleParamKey key, const std::string& value) {
    YAML::Node node(value);
    return parseNode(key, node);
}

std::string StyleParam::toString() const {

    std::string k(keyName(key));
    k += " : ";

    // TODO: cap, join and color toString()
    if (value.is<none_type>()) {
        return k + "none";
    }

    switch (key) {
    case StyleParamKey::extrude: {
        if (!value.is<glm::vec2>()) break;
        auto p = value.get<glm::vec2>();
        return k + "(" + std::to_string(p[0]) + ", " + std::to_string(p[1]) + ")";
    }
    case StyleParamKey::size:
        //TODO
        // Do String parsing for debugging
        break;
    case StyleParamKey::offset:
    case StyleParamKey::text_offset: {
        if (!value.is<glm::vec2>()) break;
        auto p = value.get<glm::vec2>();
        return k + "(" + std::to_string(p.x) + "px, " + std::to_string(p.y) + "px)";
    }
    case StyleParamKey::text_source:
    case StyleParamKey::text_source_left:
    case StyleParamKey::text_source_right:
        if (value.is<std::string>()) {
            return k + value.get<std::string>();
        } else if (value.is<TextSource>()) {
            // TODO add more..
            return k + value.get<TextSource>().keys[0];
        }
        break;
    case StyleParamKey::transition_hide_time:
    case StyleParamKey::text_transition_hide_time:
    case StyleParamKey::transition_show_time:
    case StyleParamKey::text_transition_show_time:
    case StyleParamKey::transition_selected_time:
    case StyleParamKey::text_transition_selected_time:
    case StyleParamKey::text_font_family:
    case StyleParamKey::text_font_weight:
    case StyleParamKey::text_font_style:
    case StyleParamKey::text_transform:
    case StyleParamKey::text_wrap:
    case StyleParamKey::repeat_group:
    case StyleParamKey::text_repeat_group:
    case StyleParamKey::sprite:
    case StyleParamKey::sprite_default:
    case StyleParamKey::style:
    case StyleParamKey::text_align:
    case StyleParamKey::texture:
        if (!value.is<std::string>()) break;
        return k + value.get<std::string>();
    case StyleParamKey::anchor:
    case StyleParamKey::text_anchor:
        return "[anchor]"; // TODO
    case StyleParamKey::interactive:
    case StyleParamKey::flat:
    case StyleParamKey::text_interactive:
    case StyleParamKey::tile_edges:
    case StyleParamKey::visible:
    case StyleParamKey::text_visible:
    case StyleParamKey::outline_visible:
    case StyleParamKey::collide:
    case StyleParamKey::text_optional:
    case StyleParamKey::text_collide:
        if (!value.is<bool>()) break;
        return k + std::to_string(value.get<bool>());
    case StyleParamKey::width:
    case StyleParamKey::outline_width:
    case StyleParamKey::text_font_stroke_width:
    case StyleParamKey::text_font_size:
        if (!value.is<Width>()) break;
        return k + std::to_string(value.get<Width>().value);
    case StyleParamKey::order:
    case StyleParamKey::text_order:
    case StyleParamKey::outline_order:
    case StyleParamKey::priority:
    case StyleParamKey::text_max_lines:
    case StyleParamKey::text_priority:
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::outline_style:
    case StyleParamKey::repeat_distance:
    case StyleParamKey::text_font_fill:
    case StyleParamKey::text_font_stroke_color:
    case StyleParamKey::text_repeat_distance:
    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        if (!value.is<uint32_t>()) break;
        return k + std::to_string(value.get<uint32_t>());
    case StyleParamKey::miter_limit:
    case StyleParamKey::angle:
    case StyleParamKey::outline_miter_limit:
        if (!value.is<float>()) break;
        return k + std::to_string(value.get<float>());
    default:
        break;
    }

    if (value.is<std::string>()) {
        return k + "wrong type: " + value.get<std::string>();

    }

    return k + "undefined " + std::to_string(static_cast<uint8_t>(key));

}

bool StyleParam::parseTime(const std::string &_value, float &_time) {
    ValueUnitPair result;

    if (!parseValueUnitPair(_value, result)) {
        return false;
    }

    switch (result.unit) {
        case Unit::milliseconds:
            _time = result.value / 1000.f;
            break;
        case Unit::seconds:
            _time = result.value;
            break;
        default:
            LOGW("Invalid unit provided for time %s", _value.c_str());
            return false;
            break;
    }

    return true;
}

template<typename T>
bool parseVec(const YAML::Node& node, UnitSet allowedUnits, UnitVec<T>& vec) {
    if (!node.IsSequence()) {
        return false;
    }
    size_t nodeSize = node.size();
    bool success = true;
    for (size_t i = 0; i < vec.size; i++) {
        if (i >= nodeSize) {
            success = false;
            break;
        }
        const auto& nodeElement = node[i];
        if (!nodeElement.IsScalar()) {
            success = false;
            continue;
        }
        StyleParam::ValueUnitPair result;
        success &= StyleParam::parseValueUnitPair(nodeElement.Scalar(), result);

        if (!allowedUnits.contains(result.unit)) {
            success = false;
        }
        vec.units[i] = result.unit;
        vec.value[i] = result.value;
    }

    return success;
}

bool StyleParam::parseSize(const YAML::Node& node, UnitSet allowedUnits, SizeValue& result) {
    bool success = true;
    if (node.IsScalar()) {
        success = parseSizeUnitPair(node.Scalar(), result.x);
    }
    if (node.IsSequence() && node.size() >= 2) {
        const auto& nodeFirst = node[0];
        const auto& nodeSecond = node[1];
        success &= nodeFirst.IsScalar() && parseSizeUnitPair(nodeFirst.Scalar(), result.x);
        success &= nodeSecond.IsScalar() && parseSizeUnitPair(nodeSecond.Scalar(), result.y);
    }
    return success && (allowedUnits.contains(result.x.unit) && allowedUnits.contains(result.y.unit));
}

bool StyleParam::parseVec2(const YAML::Node& node, UnitSet allowedUnits, UnitVec<glm::vec2>& result) {
    return parseVec(node, allowedUnits, result);
}

bool StyleParam::parseVec3(const YAML::Node& node, UnitSet allowedUnits, UnitVec<glm::vec3>& result) {
    return parseVec(node, allowedUnits, result);
}

bool StyleParam::parseSizeUnitPair(const std::string& value, StyleParam::ValueUnitPair& result) {
    if (value == "auto") {
        result.unit = Unit::sizeauto;
        return true;
    }
    return parseValueUnitPair(value, result);
}

bool StyleParam::parseValueUnitPair(const std::string& value, StyleParam::ValueUnitPair& result) {
    int offset = 0;
    float number = ff::stof(value.data(), value.size(), &offset);
    if (offset <= 0) {
        return false;
    }
    // Skip any leading whitespace.
    while (std::isspace(value[offset])) {
        offset++;
    }
    Unit unit = stringToUnit(value, offset, value.size() - offset);
    result.value = number;
    result.unit = unit;
    return true;
}

bool StyleParam::parseColor(const std::string& value, Color& result) {
    bool isValid = false;
    auto cssColor = CSSColorParser::parse(value, isValid);
    if (isValid) {
        result.abgr = cssColor.getInt();
    }
    return isValid;
}

bool StyleParam::parseColor(const YAML::Node& node, Color& result) {
    if (node.IsScalar()) {
        return parseColor(node.Scalar(), result);
    }
    if (node.IsSequence() && node.size() >= 3) {
        ColorF color;
        bool isValid = true;
        isValid &= YamlUtil::getFloat(node[0], color.r);
        isValid &= YamlUtil::getFloat(node[1], color.g);
        isValid &= YamlUtil::getFloat(node[2], color.b);
        if (node.size() >= 4) {
            isValid &= YamlUtil::getFloat(node[3], color.a);
        } else {
            color.a = 1.f;
        }
        if (isValid) {
            result = color.toColor();
        }
        return isValid;
    }
    return false;
}

bool StyleParam::parseFontSize(const std::string& _str, float& _pxSize) {
    if (_str.empty()) {
        return false;
    }

    int index = 0;
    float floatValue = ff::stof(_str.data(), _str.size(), &index);
    if (index <= 0) {
        return false;
    }

    _pxSize = floatValue;

    if (size_t(index) == _str.length() && (_str.find('.') == std::string::npos)) {
        return true;
    }

    size_t end = _str.length() - 1;

    if (_str.compare(index, end, "px") == 0) {
        return true;
    } else if (_str.compare(index, end, "pt") == 0) {
        _pxSize /= 0.75f;
    } else if (_str.compare(index, end, "%") == 0) {
        _pxSize /= 6.25f;
    } else if (_str.compare(index - 1, end, "em") == 0) {
        // The float parser consumes the 'e' (for scientific notation like '1.2e6') so if the string ends with 'em'
        // then 'e' will be the last character consumed and the substring from index-1 to end will be 'em'.
        _pxSize *= 16.f;
    } else {
        return false;
    }

    return true;
}

bool StyleParam::isColor(StyleParamKey _key) {
    switch (_key) {
        case StyleParamKey::color:
        case StyleParamKey::outline_color:
        case StyleParamKey::text_font_fill:
        case StyleParamKey::text_font_stroke_color:
            return true;
        default:
            return false;
    }
}

bool StyleParam::isSize(StyleParamKey _key) {
    switch (_key) {
        case StyleParamKey::size:
            return true;
        default:
            return false;
    }
}

bool StyleParam::isWidth(StyleParamKey _key) {
    switch (_key) {
        case StyleParamKey::width:
        case StyleParamKey::outline_width:
        case StyleParamKey::text_font_stroke_width:
        case StyleParamKey::placement_spacing:
            return true;
        default:
            return false;
    }
}

bool StyleParam::isOffsets(StyleParamKey _key) {
    switch (_key) {
        case StyleParamKey::offset:
        case StyleParamKey::text_offset:
            return true;
        default:
            return false;
    }
}

bool StyleParam::isFontSize(StyleParamKey _key) {
    switch (_key) {
        case StyleParamKey::text_font_size:
            return true;
        default:
            return false;
    }
}

bool StyleParam::isNumberType(StyleParamKey _key) {
    switch (_key) {
        case StyleParamKey::placement_min_length_ratio:
            return true;
        default:
            return false;
    }
}

bool StyleParam::isRequired(StyleParamKey _key) {
    static const std::vector<StyleParamKey> requiredKeys =
        { StyleParamKey::color, StyleParamKey::order, StyleParamKey::width };

    return std::find(requiredKeys.begin(), requiredKeys.end(), _key) != requiredKeys.end();
}

UnitSet StyleParam::unitSetForStyleParam(StyleParamKey key) {
    switch (key) {
    case StyleParamKey::buffer:
    case StyleParamKey::offset:
    case StyleParamKey::placement_spacing:
    case StyleParamKey::text_buffer:
    case StyleParamKey::text_font_stroke_width:
    case StyleParamKey::text_offset:
        return UnitSet{ Unit::none, Unit::pixel };
    case StyleParamKey::size:
        return UnitSet{ Unit::none, Unit::pixel, Unit::percentage, Unit::sizeauto };
    case StyleParamKey::width:
    case StyleParamKey::outline_width:
        return UnitSet{ Unit::none, Unit::pixel, Unit::meter };
    default:
        return UnitSet{};
    }
}

}
