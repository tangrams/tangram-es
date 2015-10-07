#include "sceneLayer.h"

#include <algorithm>

namespace Tangram {

SceneLayer::SceneLayer(std::string _name, Filter _filter, std::vector<DrawRule> _rules,
                       std::vector<SceneLayer> _sublayers) :
    m_filter(std::move(_filter)),
    m_name(std::move(_name)),
    m_rules(std::move(_rules)),
    m_sublayers(std::move(_sublayers)),
    m_depth(0) {

    // Rules must be sorted to merge correctly
    std::sort(m_rules.begin(), m_rules.end());

    // m_depth is one more than the maximum depth of any sublayer
    for (auto& sublayer : m_sublayers) {
        m_depth = sublayer.m_depth > m_depth ? sublayer.m_depth : m_depth;
    }
    m_depth++;

    // Sublayers must be sorted by the depth of their deepest leaf in order to
    // correctly traverse them in match()
    std::sort(m_sublayers.begin(), m_sublayers.end(),
              [&](const auto& a, const auto& b) {
                  return a.m_depth > b.m_depth;
              });

}

void SceneLayer::match(const Feature& _feat, const StyleContext& _ctx, std::vector<DrawRule>& _matches) const {

    if (!m_filter.eval(_feat, _ctx)) {
        return;
    }

    // Depth-first traversal produces correct parameter precedence when
    // sublayers are sorted by decreasing depth
    for (auto& layer : m_sublayers) {
        layer.match(_feat, _ctx, _matches);
    }

    std::vector<DrawRule> merged;

    // For all of m_rules that have the same name as an existing matched rule,
    // merge the rules; for others, take existing rule unchanged
    {
        auto myRulesIt = m_rules.begin(), myRulesEnd = m_rules.end();
        auto matchesIt = _matches.begin(), matchesEnd = _matches.end();
        while (myRulesIt != myRulesEnd && matchesIt != matchesEnd) {
            if (*myRulesIt < *matchesIt) {
                merged.push_back(*myRulesIt++);
            } else if (*matchesIt < *myRulesIt) {
                merged.push_back(std::move(*matchesIt++));
            } else {
                merged.push_back((*myRulesIt++).merge(*matchesIt++));
            }
        }
        while (myRulesIt != myRulesEnd) { merged.push_back(*myRulesIt++); }
        while (matchesIt != matchesEnd) { merged.push_back(std::move(*matchesIt++)); }
    }

    // Move merged results into output vector
    std::swap(merged, _matches);
}

}

