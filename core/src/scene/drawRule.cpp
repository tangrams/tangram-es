#include "drawRule.h"

namespace Tangram {

DrawRule::DrawRule(const std::string& _style, const std::vector<StyleParam>& _parameters) :
    style(_style),
    parameters(_parameters) {

    // Parameters within each rule must be sorted lexigraphically by key to merge correctly
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

bool DrawRule::findParameter(const std::string& _key, std::string* _out) const {

    auto it = std::lower_bound(parameters.begin(), parameters.end(), _key, [](StyleParam p, std::string k) {
        return p.key < k;
    });

    if (it->key == _key) {
        *_out = it->value;
        return true;
    }
    return false;
}

bool DrawRule::operator<(const DrawRule& _rhs) const {
    return style < _rhs.style;
}

}
