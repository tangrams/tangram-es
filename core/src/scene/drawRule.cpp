#include "drawRule.h"
#include "csscolorparser.hpp"
#include "geom.h" // for CLAMP

#include <algorithm>
#include <map>

namespace Tangram {

const StyleParam NONE;

const std::map<std::string, StyleParamKey> s_StyleParamMap = {
    {"order", StyleParamKey::order},
    {"color", StyleParamKey::color},
    {"width", StyleParamKey::width},
    {"cap", StyleParamKey::cap},
    {"join", StyleParamKey::join},
    {"outline:color", StyleParamKey::outline_color},
    {"outline:width", StyleParamKey::outline_width},
    {"outline:cap", StyleParamKey::outline_cap},
    {"outline:join", StyleParamKey::outline_join},
};

StyleParam::StyleParam(const std::string& _key, const std::string& _value) {
    auto it = s_StyleParamMap.find(_key);
    if (it == s_StyleParamMap.end()) {
        value = "";
        return;
    }

    key = it->second;
    value = _value;
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
        str += "    { " + std::to_string(static_cast<int>(p.key)) + ", " + p.value + " }\n";
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

bool DrawRule::getValue(StyleParamKey _key, std::string& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = param.value;
    return true;
};

bool DrawRule::getValue(StyleParamKey _key, float& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = std::stof(param.value);
    return true;
}

bool DrawRule::getValue(StyleParamKey _key, int32_t& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = std::stoi(param.value);
    return true;
}

bool DrawRule::getColor(StyleParamKey _key, uint32_t& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = parseColor(param.value);
    return true;
}

bool DrawRule::getLineCap(StyleParamKey _key, CapTypes& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = CapTypeFromString(param.value);
    return true;
}
bool DrawRule::getLineJoin(StyleParamKey _key, JoinTypes& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = JoinTypeFromString(param.value);
    return true;
}

bool DrawRule::operator<(const DrawRule& _rhs) const {
    return style < _rhs.style;
}

uint32_t DrawRule::parseColor(const std::string& _color) {
    uint32_t color = 0;

    if (isdigit(_color.front())) {
        // try to parse as comma-separated rgba components
        float r, g, b, a = 1.;
        if (sscanf(_color.c_str(), "%f,%f,%f,%f", &r, &g, &b, &a) >= 3) {
            color = (CLAMP(static_cast<uint32_t>(a * 255.), 0, 255)) << 24
                  | (CLAMP(static_cast<uint32_t>(r * 255.), 0, 255)) << 16
                  | (CLAMP(static_cast<uint32_t>(g * 255.), 0, 255)) << 8
                  | (CLAMP(static_cast<uint32_t>(b * 255.), 0, 255));
        }
    } else {
        // parse as css color or #hex-num
        color = CSSColorParser::parse(_color).getInt();
    }
    return color;
}

}
