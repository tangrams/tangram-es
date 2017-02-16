#include "scene/sceneLayer.h"

#include <algorithm>

namespace Tangram {

SceneLayer::SceneLayer(std::string _name, Filter _filter,
                       std::vector<DrawRuleData> _rules,
                       std::vector<SceneLayer> _sublayers,
                       bool _visible) :
    m_filter(std::move(_filter)),
    m_name(_name),
    m_rules(_rules),
    m_sublayers(std::move(_sublayers)),
    m_visible(_visible) {

    setDepth(1);

}

void SceneLayer::setDepth(size_t _d) {

    m_depth = _d;

    for (auto& layer : m_sublayers) {
        layer.setDepth(m_depth + 1);
    }

}

}
