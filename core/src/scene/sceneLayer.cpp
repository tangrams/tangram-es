#include "sceneLayer.h"

#include <algorithm>
#include <queue>

namespace Tangram {

SceneLayer::SceneLayer(std::string _name, Filter _filter,
                       std::vector<StaticDrawRule> _rules,
                       std::vector<SceneLayer> _sublayers) :
    m_filter(_filter),
    m_name(_name),
    m_rules(_rules),
    m_sublayers(_sublayers) {

    // Rules must be sorted to merge correctly - not anymore
    std::sort(m_rules.begin(), m_rules.end());
}

bool SceneLayer::match(const Feature& _feat, const StyleContext& _ctx,
                       Styling& styling) const {

    std::queue<std::vector<SceneLayer>::const_iterator> processQ;

    // not adding self to queue to avoid queue
    // allocation when there are now sublayers
    // - overoptimization?
    if (!m_filter.eval(_feat, _ctx)) {
        return false;
    }

    // add initial drawrules
    styling.mergeRules(m_rules);
    for (auto it = m_sublayers.begin(); it != m_sublayers.end(); ++it) {
        processQ.push(it);
    }

    while (!processQ.empty()) {
        const auto& layer = *processQ.front();
        processQ.pop();

        if (!layer.filter().eval(_feat, _ctx)) {
            continue;
        }

        const auto& subLayers = layer.sublayers();
        for (auto it = subLayers.begin(); it != subLayers.end(); ++it) {
            processQ.push(it);
        }
        // override with sublayer drawrules
        styling.mergeRules(layer.rules());
    }

    return true;
}

StaticDrawRule::StaticDrawRule(std::string _styleName, int _styleId, const std::vector<StyleParam>& _parameters) :
    styleName(std::move(_styleName)),
    styleId(_styleId),
    parameters(_parameters) {
}

std::string StaticDrawRule::toString() const {
    std::string str = "{\n";
    for (auto& p : parameters) {
         str += " { "
             + std::to_string(static_cast<int>(p.key))
             + ", "
             + p.toString()
             + " }\n";
    }
    str += "}\n";

    return str;
}

}
