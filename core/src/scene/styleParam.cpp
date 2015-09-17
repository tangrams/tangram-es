#include "styleParam.h"

#include "csscolorparser.hpp"
#include "platform.h"
#include "util/builders.h" // for cap, join
#include "util/geom.h" // for CLAMP
#include <algorithm>
#include <map>

namespace Tangram {

using Color = CSSColorParser::Color;

const std::map<std::string, StyleParamKey> s_StyleParamMap = {
    {"cap", StyleParamKey::cap},
    {"color", StyleParamKey::color},
    {"extrude", StyleParamKey::extrude},
    {"font:family", StyleParamKey::font_family},
    {"font:fill", StyleParamKey::font_fill},
    {"font:size", StyleParamKey::font_size},
    {"font:stroke", StyleParamKey::font_stroke},
    {"font:stroke_color", StyleParamKey::font_stroke_color},
    {"font:stroke_width", StyleParamKey::font_stroke_width},
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



StyleParam::StyleParam(const std::string& _key, const std::string& _value) {

    auto it = s_StyleParamMap.find(_key);
    if (it == s_StyleParamMap.end()) {
        logMsg("Unknown StyleParam %s:%s\n", _key.c_str(), _value.c_str());
        key = StyleParamKey::none;
        value = none_type{};
        return;
    }

    key = it->second;
    value = parseString(key, _value);
}

StyleParam::Value StyleParam::parseString(StyleParamKey key, const std::string& _value) {

    switch (key) {
    case StyleParamKey::extrude: {
        if (_value == "true") { return glm::vec2(NAN, NAN); }
        if (_value == "false") { return glm::vec2(0, 0) ; }
        auto vec2 = glm::vec2(NAN, NAN);
        if (!parseVec2(_value, {"m", "px"}, vec2)) {
            logMsg("Warning: Badly formed extrude parameter %s.\n", _value.c_str());
        }
        return vec2;
    }
    case StyleParamKey::offset:
    case StyleParamKey::size: {
        auto vec2 = glm::vec2(0.f, 0.f);
        if (!parseVec2(_value, {"px"}, vec2)) {
            logMsg("Warning: Badly formed offset parameter %s.\n", _value.c_str());
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
    case StyleParamKey::priority: {
        try {
            return static_cast<uint32_t>(std::stoi(_value));
        } catch (std::invalid_argument) {
        } catch (std::out_of_range) {}
        logMsg("Warning: Not an Integer '%s', key: '%s'",
               _value.c_str(), keyName(key));
        break;
    }
    case StyleParamKey::width:
    case StyleParamKey::outline_width:
    case StyleParamKey::font_stroke_width: {
        try {
            return static_cast<float>(std::stof(_value));
        } catch (std::invalid_argument) {
        } catch (std::out_of_range) {}
        logMsg("Warning: Not a Float '%s', key: '%s'",
               _value.c_str(), keyName(key));
        break;
    }
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::font_fill:
    case StyleParamKey::font_stroke:
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
    case StyleParamKey::priority:
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::font_fill:
    case StyleParamKey::font_stroke:
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

bool StyleParam::parseVec2(const std::string& _value, const std::vector<std::string>& _allowedUnit, glm::vec2& _vec2) {
    if (_value.empty()) {
        return false;
    }

    std::string value = _value;

    // replace all unit occurences
    for (auto& unit : _allowedUnit) {
        auto i = value.find(unit);
        // TODO: conversion
        while (i != std::string::npos) {
            value.replace(i, unit.size(), "");
            i = value.find(unit);
        }
    }

    std::replace(value.begin(), value.end(), ',', ' ');

    if (std::any_of(value.begin(), value.end(), ::isalpha)) {
        return false;
    }

    float f1, f2;
    int num = std::sscanf(value.c_str(), "%f %f", &f1, &f2);

    switch(num) {
        case 1:
            _vec2 = { f1, NAN };
            break;
        case 2:
            _vec2 = { f1, f2 };
            break;
        case 0:
        default:
            return false;
    }

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

    size_t index = 0;
    std::string kind;

    try {
        _pxSize = std::stof(_str, &index);
    } catch (std::invalid_argument) {
        return false;
    } catch (std::out_of_range) {
        return false;
    }

    if (index == _str.length() && (_str.find('.') == std::string::npos)) {
        return true;
    }

    kind = _str.substr(index, _str.length() - 1);

    if (kind == "px") {
        // px may not be fractional value
        if (_str.find('.') != std::string::npos)
            return false;
    } else if (kind == "em") {
        _pxSize *= 16.f;
    } else if (kind == "pt") {
        _pxSize /= 0.75f;
    } else if (kind == "%") {
        _pxSize /= 6.25f;
    } else {
        return false;
    }

    return true;
}

bool StyleParam::isColor(const std::string &_keyName) {
    auto it = s_StyleParamMap.find(_keyName);
    if (it == s_StyleParamMap.end()) { return false; }
    switch (it->second) {
        case StyleParamKey::color:
        case StyleParamKey::outline_color:
        case StyleParamKey::font_fill:
        case StyleParamKey::font_stroke:
        case StyleParamKey::font_stroke_color:
            return true;
        default:
            return false;
    }
}

}
