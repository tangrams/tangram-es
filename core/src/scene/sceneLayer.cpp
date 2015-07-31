#include "sceneLayer.h"
#include "style/style.h"

#include <algorithm>

namespace Tangram {

bool DrawRule::operator<(const DrawRule& _rhs) const {
    return style->getName() < _rhs.style->getName();
}

DrawRule DrawRule::merge(DrawRule& _other) const {

    decltype(parameters) merged(parameters.size() + _other.parameters.size());

    auto mIt = parameters.begin(), mEnd = parameters.end();
    auto oIt = _other.parameters.begin(), oEnd = _other.parameters.end();
    while (mIt != mEnd && oIt != oEnd) {
        auto c = mIt->first.compare(oIt->first);
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

bool DrawRule::findParameter(const std::string &_key, std::string *_out) const {

    auto it = std::lower_bound(parameters.begin(), parameters.end(), _key, [](decltype(parameters[0]) p, std::string k) {
        return p.first < k;
    });

    if (it->first == _key) {
        *_out = it->second;
        return true;
    }
    return false;
}

SceneLayer::SceneLayer(std::string _name, Filter _filter, std::vector<DrawRule> _rules, std::vector<SceneLayer> _sublayers) :
    m_filter(_filter), m_name(_name), m_rules(_rules), m_sublayers(_sublayers) {

    // Rules must be sorted to merge correctly
    std::sort(m_rules.begin(), m_rules.end());

    // Parameters within each rule must be sorted lexigraphically by key to merge correctly
    for (auto& rule : m_rules) {
        auto& params = rule.parameters;
        std::sort(params.begin(), params.end(), [](decltype(params[0]) a, decltype(params[0]) b) {
            return a.first < b.first;
        });
    }

    // m_depth is one more than the maximum depth of any sublayer
    for (auto& sublayer : m_sublayers) {
        m_depth = sublayer.m_depth ? sublayer.m_depth : m_depth;
    }
    m_depth++;

    // Sublayers must be sorted by the depth of their deepest leaf in order to correctly traverse them in match()
    std::sort(m_sublayers.begin(), m_sublayers.end(), [&](const SceneLayer& a, const SceneLayer& b) {
        return a.m_depth < b.m_depth;
    });

}

void SceneLayer::match(const Feature& _feat, const Context& _ctx, std::vector<DrawRule>& _matches) const {

    if (!m_filter.eval(_feat, _ctx)) {
        return;
    }

    // Depth-first traversal produces correct parameter precedence when sublayers are sorted by increasing depth
    for (auto& layer : m_sublayers) {
        layer.match(_feat, _ctx, _matches);
    }

    std::vector<DrawRule> merged;

    // For all of m_rules that have the same style as an existing matched rule, merge the rules;
    // for others, take existing rule unchanged
    {
        auto pIt = m_rules.begin(), pEnd = m_rules.end();
        auto cIt = _matches.begin(), cEnd = _matches.end();
        while (pIt != pEnd && cIt != cEnd) {
            if (*pIt < *cIt) {
                merged.push_back(*pIt++);
            } else if (*cIt < *pIt) {
                merged.push_back(std::move(*cIt++));
            } else {
                merged.push_back(std::move((*pIt++).merge(*cIt++)));
            }
        }
        while (pIt != pEnd) { merged.push_back(*pIt++); }
        while (cIt != cEnd) { merged.push_back(std::move(*cIt++)); }
    }

    // Move merged results into output vector
    std::swap(merged, _matches);
}

}

