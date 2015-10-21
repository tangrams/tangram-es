#include "sceneLayer.h"

#include <algorithm>

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

StaticDrawRule::StaticDrawRule(std::string _styleName, int _styleId,
                               const std::vector<StyleParam>& _parameters)
    : styleName(std::move(_styleName)),
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
