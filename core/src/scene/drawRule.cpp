#include "drawRule.h"
#include "csscolorparser.hpp"
#include "geom.h" // for CLAMP
#include "platform.h"
#include "scene/styleContext.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

namespace Tangram {

static constexpr const char* s_styleParamNames[] = {
    "none", "order", "extrude", "color", "width", "cap", "join",
    "outline:color", "outline:width", "outline:cap", "outline:join"
    "font:family", "font:weight", "font:style", "font:size", "font:fill",
    "font:stroke", "font:stroke:color", "font:stroke:width", "font:uppercase",
    "visible", "priority"
};

static constexpr const char* keyName(StyleParamKey key) {
    return s_styleParamNames[static_cast<uint8_t>(key)];
}

const StyleParam NONE;

const std::map<std::string, StyleParamKey> s_StyleParamMap = {
    {"none", StyleParamKey::none},
    {"order", StyleParamKey::order},
    {"extrude", StyleParamKey::extrude},
    {"color", StyleParamKey::color},
    {"width", StyleParamKey::width},
    {"cap", StyleParamKey::cap},
    {"join", StyleParamKey::join},
    {"outline:color", StyleParamKey::outline_color},
    {"outline:width", StyleParamKey::outline_width},
    {"outline:cap", StyleParamKey::outline_cap},
    {"outline:join", StyleParamKey::outline_join},
    {"font:family", StyleParamKey::font_family},
    {"font:weight", StyleParamKey::font_weight},
    {"font:style", StyleParamKey::font_style},
    {"font:size", StyleParamKey::font_size},
    {"font:fill", StyleParamKey::font_fill},
    {"font:stroke", StyleParamKey::font_stroke},
    {"font:stroke_color", StyleParamKey::font_stroke_color},
    {"font:stroke_width", StyleParamKey::font_stroke_width},
    {"font:uppercase", StyleParamKey::font_uppercase},
    {"visible", StyleParamKey::visible},
    {"priority", StyleParamKey::priority},
};

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
    case StyleParamKey::extrude:
        if (_value == "true") { return std::make_pair(NAN, NAN); }
        else if (_value == "false") { return std::make_pair(0.0f, 0.0f) ; }
        else {
            float f1, f2;
            int num = std::sscanf(_value.c_str(), "%f, %f", &f1, &f2);
            switch(num) {
                case 1:
                    return std::make_pair(f1, NAN);
                case 2:
                    return std::make_pair(f1, f2);
                case 0:
                default:
                    logMsg("Warning: Badly formed extrude parameter.\n");
            }
        }
        break;
    case StyleParamKey::font_family:
    case StyleParamKey::font_weight:
    case StyleParamKey::font_style:
        return _value;

    case StyleParamKey::font_size: {
        float fontSize = 16;
        if (!StyleParam::parseFontSize(_value, fontSize)) {
            logMsg("Warning: Invalid font-size '%s'.\n", _value.c_str());
        }
        return fontSize;
    }
    case StyleParamKey::font_uppercase:
    case StyleParamKey::visible:
        if (_value == "true") {
            return true;
        }
        else if (_value == "false") {
            return false;
        }
        else {
            logMsg("Warning: Bool value required for capitalized/visible. Using Default.");
        }
        break;
    case StyleParamKey::order:
    case StyleParamKey::priority: {
        try {
            return static_cast<int32_t>(std::stoi(_value));
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
        return StyleParam::parseColor(_value);

    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
        return CapTypeFromString(_value);

    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        return JoinTypeFromString(_value);

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
        if (!value.is<Extrusion>()) break;
        auto p = value.get<Extrusion>();
        return k + "(" + std::to_string(p.first) + ", " + std::to_string(p.second) + ")";
    }
    case StyleParamKey::font_family:
    case StyleParamKey::font_weight:
    case StyleParamKey::font_style:
        if (!value.is<std::string>()) break;
        return k + value.get<std::string>();
    case StyleParamKey::font_size:
        if (!value.is<float>()) break;
        return k + std::to_string(value.get<float>());
    case StyleParamKey::font_uppercase:
    case StyleParamKey::visible:
        if (!value.is<bool>()) break;
        return k + std::to_string(value.get<bool>());
    case StyleParamKey::order:
    case StyleParamKey::priority:
        if (!value.is<int32_t>()) break;
        return k + std::to_string(value.get<int32_t>());
    case StyleParamKey::width:
    case StyleParamKey::outline_width:
    case StyleParamKey::font_stroke_width:
        if (!value.is<float>()) break;
        return k + std::to_string(value.get<float>());
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::font_fill:
    case StyleParamKey::font_stroke:
    case StyleParamKey::font_stroke_color:
        if (!value.is<uint32_t>()) break;
        return k + std::to_string(value.get<uint32_t>());
    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
        if (!value.is<CapTypes>()) break;
        return k + std::to_string(static_cast<int>(value.get<CapTypes>()));
    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        if (!value.is<JoinTypes>()) break;
        return k + std::to_string(static_cast<int>(value.get<JoinTypes>()));
    case StyleParamKey::none:
        break;
    }
    return k + "undefined";
}

DrawRule::DrawRule(const std::string& _style, const std::vector<StyleParam>& _parameters) :
    style(_style),
    parameters(_parameters) {

    // Parameters within each rule must be sorted lexicographically by key to merge correctly
    std::sort(parameters.begin(), parameters.end());

}

DrawRule DrawRule::merge(DrawRule& _other) const {

    decltype(parameters) merged;

    auto myIt = parameters.begin(), myEnd = parameters.end();
    auto otherIt = _other.parameters.begin(), otherEnd = _other.parameters.end();
    while (myIt != myEnd && otherIt != otherEnd) {
        if (*myIt < *otherIt) {
            merged.push_back(*myIt++);
        } else if (*otherIt < *myIt) {
            merged.push_back(std::move(*otherIt++));
        } else {
            merged.push_back(*otherIt++);
            myIt++;
        }
    }
    while (myIt != myEnd) { merged.push_back(*myIt++); }
    while (otherIt != otherEnd) { merged.push_back(std::move(*otherIt++)); }

    return { style, merged };
}

std::string DrawRule::toString() const {

    std::string str = "{\n";

    for (auto& p : parameters) {
         str += "    { " + std::to_string(static_cast<int>(p.key)) + ", " + p.toString() + " }\n";
    }

    str += "}\n";

    return str;
}

const StyleParam&  DrawRule::findParameter(StyleParamKey _key) const {

    auto it = std::lower_bound(parameters.begin(), parameters.end(), _key,
                               [](auto& p, auto& k) { return p.key < k; });

    if (it != parameters.end() && it->key == _key) {
        return *it;
    }
    return NONE;
}

bool DrawRule::operator<(const DrawRule& _rhs) const {
    return style < _rhs.style;
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


void DrawRule::eval(const StyleContext& _ctx) {
     for (auto& param : parameters) {
         if (param.function >= 0) {
             _ctx.evalStyle(param.function, param.key, param.value);
         }
     }
}

}
