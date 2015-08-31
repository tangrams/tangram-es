#include "drawRule.h"
#include "csscolorparser.hpp"
#include "geom.h" // for CLAMP
#include "platform.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

namespace Tangram {

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
    {"font:capitalized", StyleParamKey::font_capitalized},
    {"visible", StyleParamKey::visible},
    {"priority", StyleParamKey::prioriy},
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

    switch (key) {
    case StyleParamKey::extrude:
        if (_value == "true") { value = std::make_pair(NAN, NAN); }
        else if (_value == "false") { value = std::make_pair(0.0f, 0.0f) ; }
        else {
            float f1, f2;
            int num = std::sscanf(_value.c_str(), "%f, %f", &f1, &f2);
            switch(num) {
                case 1:
                    value = std::make_pair(f1, NAN);
                    break;
                case 2:
                    value = std::make_pair(f1, f2);
                    break;
                case 0:
                default:
                    logMsg("Warning: Badly formed extrude parameter.\n");
                    break;
            }
        }
        break;
    case StyleParamKey::font_family:
    case StyleParamKey::font_weight:
    case StyleParamKey::font_style:
    case StyleParamKey::font_size:
        value = _value;
        break;
    case StyleParamKey::font_capitalized:
    case StyleParamKey::visible:
        if (_value == "true") { value = true; }
        else if (_value == "false") { value = false; }
        else {
            logMsg("Warning: Bool value required for capitalized/visible. Using Default.");
        }
        break;
    case StyleParamKey::order:
    case StyleParamKey::prioriy:
        value = static_cast<int32_t>(std::stoi(_value));
        break;
    case StyleParamKey::width:
    case StyleParamKey::outline_width:
    case StyleParamKey::font_stroke_width:
        value = static_cast<float>(std::stof(_value));
        break;
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::font_fill:
    case StyleParamKey::font_stroke:
    case StyleParamKey::font_stroke_color:
        value = DrawRule::parseColor(_value);
        break;
    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
        value = CapTypeFromString(_value);
        break;
    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        value = JoinTypeFromString(_value);
        break;
    default:
        value = none_type{};
    }
}

std::string StyleParam::toString() const {
    // TODO: cap, join and color toString()
    switch (key) {
    case StyleParamKey::extrude: {
        if (!value.is<Extrusion>()) break;
        auto p = value.get<Extrusion>();
        return "extrude : (" + std::to_string(p.first) + ", " + std::to_string(p.second) + ")";
    }
    case StyleParamKey::font_family:
    case StyleParamKey::font_weight:
    case StyleParamKey::font_style:
    case StyleParamKey::font_size:
        if (!value.is<std::string>()) break;
        return value.get<std::string>();
    case StyleParamKey::font_capitalized:
    case StyleParamKey::visible:
        if (!value.is<bool>()) break;
        return std::to_string(value.get<bool>());
    case StyleParamKey::order:
    case StyleParamKey::prioriy:
        if (!value.is<int32_t>()) break;
        return "order : " + std::to_string(value.get<int32_t>());
    case StyleParamKey::width:
    case StyleParamKey::outline_width:
    case StyleParamKey::font_stroke_width:
        if (!value.is<float>()) break;
        return "width : " + std::to_string(value.get<float>());
    case StyleParamKey::color:
    case StyleParamKey::outline_color:
    case StyleParamKey::font_fill:
    case StyleParamKey::font_stroke:
    case StyleParamKey::font_stroke_color:
        if (!value.is<Color>()) break;
        return "color : " + std::to_string(value.get<Color>().getInt());
    case StyleParamKey::cap:
    case StyleParamKey::outline_cap:
        if (!value.is<CapTypes>()) break;
        return "cap : " + std::to_string(static_cast<int>(value.get<CapTypes>()));
    case StyleParamKey::join:
    case StyleParamKey::outline_join:
        if (!value.is<JoinTypes>()) break;
        return "join : " + std::to_string(static_cast<int>(value.get<JoinTypes>()));
    case StyleParamKey::none:
        break;
    }
    return "undefined";
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

Color DrawRule::parseColor(const std::string& _color) {
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
    return color;
}

}
