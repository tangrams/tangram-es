#include "sceneLayer.h"

#include <algorithm>

namespace Tangram {

SceneLayer::SceneLayer(std::string _name, Filter _filter,
                       std::vector<DrawRuleData> _rules,
                       std::vector<SceneLayer> _sublayers) :
    m_filter(_filter),
    m_name(_name),
    m_rules(_rules),
    m_sublayers(_sublayers) {

    setDepth(1);

}

void SceneLayer::setDepth(size_t _d) {

    m_depth = _d;

    for (auto& layer : m_sublayers) {
        layer.setDepth(m_depth + 1);
    }

}

}
