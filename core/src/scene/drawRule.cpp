#include "drawRule.h"
#include "platform.h"
#include "scene/stops.h"
#include "scene/styleContext.h"

#include <algorithm>

namespace Tangram {

const StyleParam NONE;

DrawRule::DrawRule(const std::string& _style, const std::vector<StyleParam>& _parameters, bool _sorted) :
    style(_style),
    parameters(_parameters) {

    if (!_sorted) {
        // Parameters within each rule must be sorted lexicographically by key to merge correctly
        std::sort(parameters.begin(), parameters.end());
    }

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

    return { style, merged, true };
}

std::string DrawRule::toString() const {

    std::string str = "{\n";

    for (auto& p : parameters) {
         str += "    { " + std::to_string(static_cast<int>(p.key)) + ", " + p.toString() + " }\n";
    }

    str += "}\n";

    return str;
}

const StyleParam& DrawRule::findParameter(StyleParamKey _key) const {

    auto it = std::lower_bound(parameters.begin(), parameters.end(), _key,
                               [](auto& p, auto& k) { return p.key < k; });

    if (it != parameters.end() && it->key == _key) {
        return *it;
    }
    return NONE;
}

bool DrawRule::isJSFunction(StyleParamKey _key) const {
    auto& param = findParameter(_key);
    if (!param) {
        return false;
    }
    return param.function >= 0;
}

bool DrawRule::operator<(const DrawRule& _rhs) const {
    return style < _rhs.style;
}

void DrawRule::eval(const StyleContext& _ctx) {
     for (auto& param : parameters) {
         if (param.function >= 0) {
             _ctx.evalStyle(param.function, param.key, param.value);
         }
         if (param.stops) {
             float z = _ctx.getGlobal("$zoom").get<float>();
             if (param.value.is<float>()) {
                 param.value = param.stops->evalFloat(z);
             } else if (param.value.is<uint32_t>()) {
                 param.value = param.stops->evalColor(z);
             }
         }
     }
}

}
