#include "scene/styleParam.h"

#include "log.h"
#include "platform.h"
#include "style/textStyle.h"
#include "util/builders.h" // for cap, join
#include "util/extrude.h"
#include "util/floatFormatter.h"
#include "util/geom.h" // for CLAMP

#include "csscolorparser.hpp"
#include <algorithm>
#include <cstring>
#include <map>

namespace Tangram {

using Color = CSSColorParser::Color;

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

const std::map<StyleParamKey, std::vector<Unit>> s_StyleParamUnits = {
    {StyleParamKey::offset, {Unit::pixel}},
    {StyleParamKey::text_offset, {Unit::pixel}},
    {StyleParamKey::buffer, {Unit::pixel}},
    {StyleParamKey::text_buffer, {Unit::pixel}},
    {StyleParamKey::size, {Unit::pixel}},
    {StyleParamKey::placement_spacing, {Unit::pixel}},
    {StyleParamKey::text_font_stroke_width, {Unit::pixel}},
    {StyleParamKey::width, {Unit::meter, Unit::pixel}},
    {StyleParamKey::outline_width, {Unit::meter, Unit::pixel}}
};

static int parseInt(const std::string& _str, int& _value) {
    try {
        size_t index;
        _value = std::stoi(_str, &index);
        return index;
    } catch (std::invalid_argument) {
    } catch (std::out_of_range) {}
    LOGW("Not an Integer '%s'", _str.c_str());

    return -1;
}

static int parseFloat(const std::string& _str, double& _value) {
    int index = 0;
    _value = ff::stod(_str.data(), _str.size(), &index);
    if (index == 0) {
        LOGW("Not a Float '%s'", _str.c_str());
        return -1;
    }

    return index;
}

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

StyleParam::StyleParam(const std::string& _key, const std::string& _value) {
    key = getKey(_key);
    value = none_type{};

    if (key == StyleParamKey::none) {
        LOGW("Unknown StyleParam %s:%s", _key.c_str(), _value.c_str());
        return;
    }
    if (!_value.empty()) {
        value = parseString(key, _value);
    }
}

StyleParam::Value StyleParam::parseString(StyleParamKey key, const std::string& _value) {
    switch (key) {
    case StyleParamKey::extrude: {
        return parseExtrudeString(_value);
    }
    case StyleParamKey::text_wrap: {
        int textWrap;
        if (_value == "false") {
            return std::numeric_limits<uint32_t>::max();
        }
        if (_value == "true") {
            return uint32_t(15); // DEFAULT
        }
        if (parseInt(_value, textWrap) > 0 && textWrap > 0) {
             return static_cast<uint32_t>(textWrap);
        }
        return std::numeric_limits<uint32_t>::max();
    }
    case StyleParamKey::text_offset:
    case StyleParamKey::offset: {
        UnitVec<glm::vec2> vec;
        if (!parseVec2(_value, { Unit::pixel }, vec) || std::isnan(vec.value.y)) {
            LOGW("Invalid offset parameter '%s'.", _value.c_str());
        }
        return vec.value;
    }
    case StyleParamKey::text_buffer:
    case StyleParamKey::buffer: {
        UnitVec<glm::vec2> vec;
        if (!parseVec2(_value, { Unit::pixel }, vec)) {
            LOGW("Invalid buffer parameter '%s'.", _value.c_str());
        }
        if (std::isnan(vec.value.y)) {
            vec.value.y = vec.value.x;
        }
        return vec.value;
    }
    case StyleParamKey::size: {
        UnitVec<glm::vec2> vec;
        if (!parseVec2(_value, { Unit::pixel }, vec)) {
            LOGW("Invalid size parameter '%s'.", _value.c_str());
        }
        return vec.value;
    }
    case StyleParamKey::transition_hide_time:
    case StyleParamKey::transition_show_time:
    case StyleParamKey::transition_selected_time:
    case StyleParamKey::text_transition_hide_time:
    case StyleParamKey::text_transition_show_time:
    case StyleParamKey::text_transition_selected_time: {
        float time = 0.0f;
        if (!parseTime(_value, time)) {
            LOGW("Invalid time param '%s'", _value.c_str());
        }
        return time;
    }
    case StyleParamKey::text_font_family:
    case StyleParamKey::text_font_weight:
    case StyleParamKey::text_font_style: {
        std::string normalized = _value;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        return normalized;
    }
    case StyleParamKey::anchor:
    case StyleParamKey::text_anchor: {
        LabelProperty::Anchors anchors;
        for (auto& anchor : splitString(_value, ',')) {
            if (anchors.count == LabelProperty::max_anchors) { break; }

            LabelProperty::Anchor labelAnchor;
            if (LabelProperty::anchor(anchor, labelAnchor)) {
                anchors.anchor[anchors.count++] = labelAnchor;
            } else {
                LOG("Invalid anchor %s", anchor.c_str());
            }
        }
        return anchors;
    }
    case StyleParamKey::placement: {
        LabelProperty::Placement placement = LabelProperty::Placement::vertex;
        if (!LabelProperty::placement(_value, placement)) {
            LOG("Invalid placement parameter, Setting vertex as default.");
        }
        return placement;
    }
    case StyleParamKey::text_source:
    case StyleParamKey::text_source_left:
    case StyleParamKey::text_source_right: {
        TextSource textSource;
        // FIXME remove white space
        std::string tmp;
        if (_value.find(',') != std::string::npos) {
            std::stringstream ss(_value);
            while (std::getline(ss, tmp, ',')) {
                textSource.keys.push_back(tmp);
            }
        } else {
            textSource.keys.push_back(_value);
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
    case StyleParamKey::texture:
        return _value;
    case StyleParamKey::text_font_size: {
        float fontSize = 0.f;
        if (!parseFontSize(_value, fontSize)) {
            LOGW("Invalid font-size '%s'.", _value.c_str());
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
    case StyleParamKey::text_collide:
        if (_value == "true") { return true; }
        if (_value == "false") { return false; }
        LOGW("Invalid boolean value %s for key %s", _value.c_str(), StyleParam::keyName(key).c_str());
        break;
    case StyleParamKey::text_order:
        LOGW("text:order parameter is ignored.");
        break;
    case StyleParamKey::order:
    case StyleParamKey::outline_order:
    case StyleParamKey::priority:
    case StyleParamKey::text_max_lines:
    case StyleParamKey::text_priority: {
        int num;
        if (parseInt(_value, num) > 0) {
            if (num >= 0) {
                return static_cast<uint32_t>(num);
            }
        }
        LOGW("Invalid '%s' value '%s'", keyName(key).c_str(), _value.c_str());
        break;
    }
    case StyleParamKey::placement_spacing: {
        ValueUnitPair placementSpacing;
        placementSpacing.unit = Unit::pixel;

        int pos = parseValueUnitPair(_value, 0, placementSpacing);
        if (pos < 0) {
            LOGW("Invalid placement spacing value '%s'", _value.c_str());
            placementSpacing.value =  80.0f;
            placementSpacing.unit = Unit::pixel;
        } else {
            if (placementSpacing.unit != Unit::pixel) {
                LOGW("Invalid unit provided for placement spacing");
            }
        }

        return Width(placementSpacing);
    }
    case StyleParamKey::repeat_distance:
    case StyleParamKey::text_repeat_distance: {
        ValueUnitPair repeatDistance;
        repeatDistance.unit = Unit::pixel;

        int pos = parseValueUnitPair(_value, 0, repeatDistance);
        if (pos < 0) {
            LOGW("Invalid repeat distance value '%s'", _value.c_str());
            repeatDistance.value =  256.0f;
            repeatDistance.unit = Unit::pixel;
        } else {
            if (repeatDistance.unit != Unit::pixel) {
                LOGW("Invalid unit provided for repeat distance");
            }
        }

        return Width(repeatDistance);
    }
    case StyleParamKey::width:
    case StyleParamKey::outline_width: {
        ValueUnitPair width;
        width.unit = Unit::meter;

        int pos = parseValueUnitPair(_value, 0, width);
        if (pos < 0) {
            LOGW("Invalid width value '%s'", _value.c_str());
            width.value =  2.0f;
            width.unit = Unit::pixel;
        }

        return Width(width);
    }
    case StyleParamKey::angle: {
        double num;
        if (_value == "auto") {
            return std::nanf("1");
        } else if (parseFloat(_value, num) > 0) {
            return static_cast<float>(num);
        }
        break;
    }
    case StyleParamKey::miter_limit:
    case StyleParamKey::outline_miter_limit:
    case StyleParamKey::placement_min_length_ratio:
    case StyleParamKey::text_font_stroke_width: {
        double num;
        if (parseFloat(_value, num) > 0) {
             return static_cast<float>(num);
        }
        break;
    }

    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::text_font_fill:
    case StyleParamKey::text_font_stroke_color:
        return parseColor(_value);

    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
        return static_cast<uint32_t>(CapTypeFromString(_value));

    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        return static_cast<uint32_t>(JoinTypeFromString(_value));

    default:
        break;
    }

    return none_type{};
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

static const std::vector<std::string> s_units = { "px", "ms", "m", "s" };

int StyleParam::parseValueUnitPair(const std::string& _value, size_t offset,
                                   StyleParam::ValueUnitPair& _result) {

    if (offset >= _value.length()) { return -1; }

    const char* str = _value.c_str();
    int count;
    _result.value = ff::stof(str + offset,
                             _value.length() - offset, &count);

    if (count == 0) { return -1; }
    offset += count;

    if (offset >= _value.length()) { return offset; }

    for (size_t i = 0; i < s_units.size(); ++i) {
        const auto& unit = s_units[i];
        std::string valueUnit;
        if (unit == _value.substr(offset, std::min<int>(_value.length(), unit.length()))) {
            _result.unit = static_cast<Unit>(i);
            offset += unit.length();
            break;
        }
    }

    // Skip next comma
    while (str[offset] == ' ') { offset++; }
    if (str[offset] == ',') { offset++; }

    return offset;
}

bool StyleParam::parseTime(const std::string &_value, float &_time) {
    ValueUnitPair p;

    if (!parseValueUnitPair(_value, 0, p)) {
        return false;
    }

    switch (p.unit) {
        case Unit::milliseconds:
            _time = p.value / 1000.f;
            break;
        case Unit::seconds:
            _time = p.value;
            break;
        default:
            LOGW("Invalid unit provided for time %s", _value.c_str());
            return false;
            break;
    }

    return true;
}

template<typename T>
int parseVec(const std::string& _value, T& _vec) {

    const size_t elements = _vec.length();
    const char* str = _value.data();

    const int length = _value.length();
    int count = 0;
    int offset = 0;

    for (size_t i = 0; i < elements; i++) {

        float v = ff::stof(str + offset, length - offset, &count);
        if (count == 0) { return i; }

        _vec[i] = v;
        offset += count;

        if (length - offset <= 0) { return i; }

        // Skip next comma
        while (str[offset] == ' ') { offset++; }
        if (str[offset++] != ',') { return i; }
    }

    return elements;
}

template<typename T>
int parseVec(const std::string& _value, const std::vector<Unit>& units, UnitVec<T>& _vec) {
     // initialize with defaults
    const size_t elements = _vec.size;
    for (size_t i = 0; i < elements; i++) {
        _vec.units[i] = units[0];
        _vec.value[i] = NAN;
    }

    int offset = 0;
    for (size_t i = 0; i < elements; i++) {
        StyleParam::ValueUnitPair v;
        offset = StyleParam::parseValueUnitPair(_value, offset, v);
        if (offset < 0) { return i; }

        if (std::find(units.begin(), units.end(), v.unit) == units.end()) {
            return 0;
        }
        _vec.units[i] = v.unit;
        _vec.value[i] = v.value;
    }

    return elements;
}

bool StyleParam::parseVec2(const std::string& _value, const std::vector<Unit>& units, UnitVec<glm::vec2>& _vec) {
    return parseVec(_value, units, _vec);
}

bool StyleParam::parseVec3(const std::string& _value, const std::vector<Unit>& units, UnitVec<glm::vec3> & _vec) {
    return parseVec(_value, units, _vec);
}

uint32_t StyleParam::parseColor(const std::string& _color) {
    Color color;

    // First, try to parse as comma-separated rgba components.
    glm::vec4 rgba(1.0f);
    int elements = parseVec(_color, rgba);

    if (elements >= 3) {
        color = Color {
            static_cast<uint8_t>(CLAMP((rgba[0] * 255.), 0, 255)),
            static_cast<uint8_t>(CLAMP((rgba[1] * 255.), 0, 255)),
            static_cast<uint8_t>(CLAMP((rgba[2] * 255.), 0, 255)),
            CLAMP(rgba[3], 0, 1)
        };
    } else {
        // parse as css color or #hex-num
        color = CSSColorParser::parse(_color);
    }
    return color.getInt();
}

bool StyleParam::parseFontSize(const std::string& _str, float& _pxSize) {
    if (_str.empty()) {
        return false;
    }

    double num;
    int index = parseFloat(_str, num);
    if (index < 0) { return false; }

    _pxSize = static_cast<float>(num);

    if (size_t(index) == _str.length() && (_str.find('.') == std::string::npos)) {
        return true;
    }

    size_t end = _str.length() - 1;

    if (_str.compare(index, end, "px") == 0) {
        return true;
    } else if (_str.compare(index, end, "em") == 0) {
        _pxSize *= 16.f;
    } else if (_str.compare(index, end, "pt") == 0) {
        _pxSize /= 0.75f;
    } else if (_str.compare(index, end, "%") == 0) {
        _pxSize /= 6.25f;
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

const std::vector<Unit>& StyleParam::unitsForStyleParam(StyleParamKey _key) {
    auto it = s_StyleParamUnits.find(_key);
    if (it != s_StyleParamUnits.end()) {
        return it->second;
    }
    static const std::vector<Unit> empty;
    return empty;
}

}
