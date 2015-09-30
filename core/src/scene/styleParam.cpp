#include "styleParam.h"

#include "csscolorparser.hpp"
#include "platform.h"
#include "util/builders.h" // for cap, join
#include "util/geom.h" // for CLAMP
#include <algorithm>
#include <map>
#include <cstring>

namespace Tangram {

using Color = CSSColorParser::Color;

const std::map<std::string, StyleParamKey> s_StyleParamMap = {
    {"cap", StyleParamKey::cap},
    {"color", StyleParamKey::color},
    {"extrude", StyleParamKey::extrude},
    {"font:family", StyleParamKey::font_family},
    {"font:fill", StyleParamKey::font_fill},
    {"font:size", StyleParamKey::font_size},
    {"font:stroke:color", StyleParamKey::font_stroke_color},
    {"font:stroke:width", StyleParamKey::font_stroke_width},
    {"font:style", StyleParamKey::font_style},
    {"font:weight", StyleParamKey::font_weight},
    {"join", StyleParamKey::join},
    {"none", StyleParamKey::none},
    {"offset", StyleParamKey::offset},
    {"order", StyleParamKey::order},
    {"outline:cap", StyleParamKey::outline_cap},
    {"outline:color", StyleParamKey::outline_color},
    {"outline:join", StyleParamKey::outline_join},
    {"outline:width", StyleParamKey::outline_width},
    {"outline:order", StyleParamKey::outline_order},
    {"priority", StyleParamKey::priority},
    {"sprite", StyleParamKey::sprite},
    {"sprite_default", StyleParamKey::sprite_default},
    {"size", StyleParamKey::size},
    {"text_source", StyleParamKey::text_source},
    {"transform", StyleParamKey::transform},
    {"visible", StyleParamKey::visible},
    {"width", StyleParamKey::width},
};

static const char* keyName(StyleParamKey key) {
    static std::string empty = "";
    for (const auto& entry : s_StyleParamMap) {
        if (entry.second == key) { return entry.first.c_str(); }
    }
    return empty.c_str();
}

static int parseInt(const std::string& _str, int& _value) {
    try {
        size_t index;
        _value = std::stoi(_str, &index);
        return index;
    } catch (std::invalid_argument) {
    } catch (std::out_of_range) {}
    logMsg("Warning: Not an Integer '%s'", _str.c_str());

    return -1;
}

static int parseFloat(const std::string& _str, double& _value) {
    try {
        size_t index;
        _value = std::stof(_str, &index);
        return index;
    } catch (std::invalid_argument) {
    } catch (std::out_of_range) {}
    logMsg("Warning: Not a Float '%s'", _str.c_str());

    return -1;
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
        logMsg("Unknown StyleParam %s:%s\n", _key.c_str(), _value.c_str());
        return;
    }
    if (!_value.empty()) {
        value = parseString(key, _value);
    }
}

StyleParam::Value StyleParam::parseString(StyleParamKey key, const std::string& _value) {

    switch (key) {
    case StyleParamKey::extrude: {
        if (_value == "true") { return glm::vec2(NAN, NAN); }
        if (_value == "false") { return glm::vec2(0, 0) ; }
        auto vec2 = glm::vec2(NAN, NAN);
        if (!parseVec2(_value, { Unit::meter, Unit::pixel }, vec2)) {
            logMsg("Warning: Invalid extrude parameter '%s'.\n", _value.c_str());
        }
        return vec2;
    }
    case StyleParamKey::offset: {
        auto vec2 = glm::vec2(0.f, 0.f);
        if (!parseVec2(_value, { Unit::pixel }, vec2) || isnan(vec2.y)) {
            logMsg("Warning: Invalid offset parameter '%s'.\n", _value.c_str());
        }
        return vec2;
    }
    case StyleParamKey::size: {
        auto vec2 = glm::vec2(0.f, 0.f);
        if (!parseVec2(_value, { Unit::pixel }, vec2)) {
            logMsg("Warning: Invalid size parameter '%s'.\n", _value.c_str());
        }
        return vec2;
    }
    case StyleParamKey::font_family:
    case StyleParamKey::font_weight:
    case StyleParamKey::font_style:
    case StyleParamKey::text_source:
    case StyleParamKey::transform:
    case StyleParamKey::sprite:
    case StyleParamKey::sprite_default:
        return _value;
    case StyleParamKey::font_size: {
        float fontSize = 16;
        if (!parseFontSize(_value, fontSize)) {
            logMsg("Warning: Invalid font-size '%s'.\n", _value.c_str());
        }
        return fontSize;
    }
    case StyleParamKey::visible:
        if (_value == "true") { return true; }
        if (_value == "false") { return false; }
        logMsg("Warning: Bool value required for capitalized/visible. Using Default.");
        break;
    case StyleParamKey::order:
    case StyleParamKey::outline_order:
    case StyleParamKey::priority: {
        int num;
        if (parseInt(_value, num) > 0) {
             return static_cast<uint32_t>(num);
        }
        break;
    }
    case StyleParamKey::width:
    case StyleParamKey::outline_width:
    case StyleParamKey::font_stroke_width: {
        double num;
        if (parseFloat(_value, num) > 0) {
             return static_cast<float>(num);
        }
        break;
    }
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::font_fill:
    case StyleParamKey::font_stroke_color:
        return parseColor(_value);

    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
        return static_cast<uint32_t>(CapTypeFromString(_value));

    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        return static_cast<uint32_t>(JoinTypeFromString(_value));

    case StyleParamKey::none:
        break;
    }

    return none_type{};
}

std::string StyleParam::toString() const {

    std::string k(keyName(key));
    k += " : ";

    // TODO: cap, join and color toString()
    if (value.is<none_type>()) {
        return k + "undefined";
    }

    switch (key) {
    case StyleParamKey::extrude: {
        if (!value.is<glm::vec2>()) break;
        auto p = value.get<glm::vec2>();
        return k + "(" + std::to_string(p[0]) + ", " + std::to_string(p[1]) + ")";
    }
    case StyleParamKey::size:
    case StyleParamKey::offset: {
        if (!value.is<glm::vec2>()) break;
        auto p = value.get<glm::vec2>();
        return k + "(" + std::to_string(p.x) + "px, " + std::to_string(p.y) + "px)";
    }
    case StyleParamKey::font_family:
    case StyleParamKey::font_weight:
    case StyleParamKey::font_style:
    case StyleParamKey::text_source:
    case StyleParamKey::transform:
    case StyleParamKey::sprite:
    case StyleParamKey::sprite_default:
        if (!value.is<std::string>()) break;
        return k + value.get<std::string>();
    case StyleParamKey::visible:
        if (!value.is<bool>()) break;
        return k + std::to_string(value.get<bool>());
    case StyleParamKey::width:
    case StyleParamKey::outline_width:
    case StyleParamKey::font_stroke_width:
    case StyleParamKey::font_size:
        if (!value.is<float>()) break;
        return k + std::to_string(value.get<float>());
    case StyleParamKey::order:
    case StyleParamKey::outline_order:
    case StyleParamKey::priority:
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::font_fill:
    case StyleParamKey::font_stroke_color:
    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        if (!value.is<uint32_t>()) break;
        return k + std::to_string(value.get<uint32_t>());
    case StyleParamKey::none:
        break;
    }
    return k + "undefined";
}

int parseValueUnitPair(const std::string& _value, size_t start,
                       StyleParam::ValueUnitPair& _result) {

    static const char* units[] = { "px", "m" };
    static const size_t ulen[] = { 2, 1 };

    if (start >= _value.length()) { return -1; }

    float num;
    int end;

    int ok = std::sscanf(_value.c_str() + start, "%f%n", &num, &end);

    if (!ok) { return -1; }

    _result.value = static_cast<float>(num);

    start += end;

    if (start >= _value.length()) { return start; }

    for (int i = 0; i < 2; i++) {
        if (_value.compare(start, ulen[i], units[i]) == 0) {
            _result.unit = static_cast<Unit>(i);
            start += ulen[i];
            break;
        }
    }

    // TODO skip whitespace , whitespace
    return std::min(_value.length(), start + 1);
}

bool StyleParam::parseVec2(const std::string& _value, const std::vector<Unit> units, glm::vec2& _vec) {
    ValueUnitPair v1, v2;

    // initialize with defaults
    v1.unit = v2.unit = units[0];

    int pos = parseValueUnitPair(_value, 0, v1);
    if (pos < 0) {
        return false;
    }

    if (std::find(units.begin(), units.end(), v1.unit) == units.end()) {
        return false;
    }

    pos = parseValueUnitPair(_value, pos, v2);
    if (pos < 0) {
        _vec = { v1.value, NAN };
        return true;
    }

    if (std::find(units.begin(), units.end(), v1.unit) == units.end()) {
        return false;
    }

    _vec = { v1.value, v2.value };
    return true;
}

uint32_t StyleParam::parseColor(const std::string& _color) {
    Color color;

    if (isdigit(_color.front())) {
        // try to parse as comma-separated rgba components
        float r, g, b, a = 1.;
        if (sscanf(_color.c_str(), "%f,%f,%f,%f", &r, &g, &b, &a) >= 3) {
            color = Color {
                static_cast<uint8_t>(CLAMP((r * 255.), 0, 255)),
                static_cast<uint8_t>(CLAMP((g * 255.), 0, 255)),
                static_cast<uint8_t>(CLAMP((b * 255.), 0, 255)),
                CLAMP(a, 0, 1)
            };
        }
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
        // px may not be fractional value here
        if (_str.find('.') != std::string::npos)
            return false;
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
        case StyleParamKey::font_fill:
        case StyleParamKey::font_stroke_color:
            return true;
        default:
            return false;
    }
}

}
