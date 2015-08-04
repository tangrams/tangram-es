#include "drawRule.h"
#include "csscolorparser.hpp"
#include "geom.h" // for CLAMP

#include <algorithm>

namespace Tangram {

const StyleParam StyleParam::NONE;

DrawRule::DrawRule(const std::string& _style, const std::vector<StyleParam>& _parameters) :
    style(_style),
    parameters(_parameters) {

    // Parameters within each rule must be sorted lexicographically by key to merge correctly
    std::sort(parameters.begin(), parameters.end());

}

DrawRule DrawRule::merge(DrawRule& _other) const {

    decltype(parameters) merged;

    auto mIt = parameters.begin(), mEnd = parameters.end();
    auto oIt = _other.parameters.begin(), oEnd = _other.parameters.end();
    while (mIt != mEnd && oIt != oEnd) {
        auto c = mIt->key.compare(oIt->key);
        if (c < 0) {
            merged.push_back(*mIt++);
        } else if (c > 0) {
            merged.push_back(std::move(*oIt++));
        } else {
            merged.push_back(*oIt++);
            mIt++;
        }
    }
    while (mIt != mEnd) { merged.push_back(*mIt++); }
    while (oIt != oEnd) { merged.push_back(std::move(*oIt++)); }

    return { style, merged };
}

std::string DrawRule::toString() const {

    std::string str = "{\n";

    for (auto& p : parameters) {
        str += "    { " + p.key + ", " + p.value + " }\n";
    }

    str += "}\n";

    return str;
}

const StyleParam&  DrawRule::findParameter(const char* _key) const {

    auto it = std::lower_bound(parameters.begin(), parameters.end(), _key,
                               [](auto& p, auto& k) { return p.key < k; });

    if (it != parameters.end() && it->key == _key) {
        return *it;
    }
    return StyleParam::NONE;
}

bool DrawRule::getValue(const char* _key, float& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = std::stof(param.value);
    return true;
}

bool DrawRule::getValue(const char* _key, int32_t& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = std::stoi(param.value);
    return true;
}

bool DrawRule::getColor(const char* _key, uint32_t& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = parseColor(param.value);
    return true;
}

bool DrawRule::getLineCap(const char* _key, CapTypes& _value) const {
    auto& param = findParameter(_key);
    if (!param) { return false; }
    _value = CapTypeFromString(param.value);
    return true;
}
bool DrawRule::getLineJoin(const char* _key, JoinTypes& _value) const {
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
