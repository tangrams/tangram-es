#include "sceneLayer.h"

#include <algorithm>
#include <queue>

namespace Tangram {

SceneLayer::SceneLayer(std::string _name, Filter _filter, std::vector<DrawRule> _rules,
                       std::vector<SceneLayer> _sublayers) :
    m_filter(_filter),
    m_name(_name),
    m_rules(_rules),
    m_sublayers(_sublayers) {

    // Rules must be sorted to merge correctly
    std::sort(m_rules.begin(), m_rules.end());

}

std::vector<DrawRule> SceneLayer::match(const Feature& _feat, const StyleContext& _ctx) const {

    std::vector<DrawRule> matches;
    std::queue<std::vector<SceneLayer>::const_iterator> processQ;

    if (!m_filter.eval(_feat, _ctx)) {
        return matches;
    }

    matches.insert(matches.end(), rules().begin(), rules().end());
    for (auto iter = m_sublayers.begin(); iter != m_sublayers.end(); ++iter) {
        processQ.push(iter);
    }

    while (!processQ.empty()) {

        const auto& layer = *processQ.front();
        if (!layer.filter().eval(_feat, _ctx)) {
            processQ.pop();
            continue;
        }

        const auto& subLayers = layer.sublayers();
        for (auto iter = subLayers.begin(); iter != subLayers.end(); ++iter) {
            processQ.push(iter);
        }
        processQ.pop();

        const auto& rules = layer.rules();

        {
            std::vector<DrawRule> merged;
            merged.reserve(rules.size() + matches.size());
            auto myRulesIt = rules.begin(), myRulesEnd = rules.end();
            auto matchesIt = matches.begin(), matchesEnd = matches.end();
            while (myRulesIt != myRulesEnd && matchesIt != matchesEnd) {
                if (*myRulesIt < *matchesIt) {
                    merged.push_back(*myRulesIt++);
                } else if (*matchesIt < *myRulesIt) {
                    merged.push_back(std::move(*matchesIt++));
                } else {
                    //merge parent properties, retain self properties
                    merged.push_back((*myRulesIt++).merge(*matchesIt++));
                }
            }
            while (myRulesIt != myRulesEnd) { merged.push_back(*myRulesIt++); }
            while (matchesIt != matchesEnd) { merged.push_back(std::move(*matchesIt++)); }
            matches.swap(merged);
        }
    }

    return matches;

}

}

