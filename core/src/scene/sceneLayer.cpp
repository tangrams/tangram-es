#include "sceneLayer.h"
#include "style/style.h"

#include <algorithm>

namespace Tangram {

uint8_t SceneLayer::s_layerCount = 0;

SceneLayer::SceneLayer(std::string _name, Filter _filter, std::vector<DrawRule> _rules, std::vector<SceneLayer> _sublayers) :
    m_filter(_filter), m_name(_name), m_rules(_rules), m_sublayers(_sublayers) {

    m_id = s_layerCount++;

    std::sort(m_rules.begin(), m_rules.end());

}

void SceneLayer::match(const Feature& _feat, const Context& _ctx, std::vector<DrawRule>& _matches) const {

    if (!m_filter.eval(_feat, _ctx)) {
        return;
    }

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

bool DrawRule::operator<(const DrawRule& _rhs) const {
    return style->getName() < _rhs.style->getName();
}

DrawRule DrawRule::merge(DrawRule& _other) const {

    StyleParamMap merged(parameters);

    for (auto& entry : _other.parameters) {
        // Override 'parent' values with 'child' values
        merged.insert(std::move(entry));
    }

    return { style, merged };
}

}

