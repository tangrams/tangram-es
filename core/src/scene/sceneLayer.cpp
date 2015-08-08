#include "sceneLayer.h"

#include <algorithm>

namespace Tangram {

SceneLayer::SceneLayer(std::string _name, Filter _filter, std::vector<DrawRule> _rules, std::vector<SceneLayer> _sublayers) :
    m_filter(_filter), m_name(_name), m_rules(_rules), m_sublayers(_sublayers), m_depth(0) {

    // Rules must be sorted to merge correctly
    std::sort(m_rules.begin(), m_rules.end());

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

