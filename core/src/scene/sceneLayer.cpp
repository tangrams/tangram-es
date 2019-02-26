#include "scene/sceneLayer.h"

#include <algorithm>
#include <type_traits>

namespace Tangram {

static_assert(std::is_move_constructible<SceneLayer>::value, "check");

SceneLayer::SceneLayer(std::string _name, Filter _filter,
                       std::vector<DrawRuleData> _rules,
                       std::vector<SceneLayer> _sublayers,
                       int _priority,
                       bool _enabled,
                       bool _exclusive) :
    m_filter(std::move(_filter)),
    m_name(_name),
    m_rules(_rules),
    m_sublayers(std::move(_sublayers)),
    m_priority(_priority),
    m_enabled(_enabled),
    m_exclusive(_exclusive) {

    setDepth(1);

}

void SceneLayer::setDepth(size_t _d) {

    m_depth = _d;

    for (auto& layer : m_sublayers) {
        layer.setDepth(m_depth + 1);
    }

}

void SceneLayer::sortSublayers() {

    if (m_subLayersSorted) { return; }
    m_subLayersSorted = true;

    std::sort(m_sublayers.begin(), m_sublayers.end(),
            [&](const SceneLayer& a, const SceneLayer& b) {
                if (a.exclusive() && !b.exclusive()) {
                    return true;
                } else if (!a.exclusive() && b.exclusive()) {
                    return false;
                } else {
                    return a.priority() <= b.priority();
                }
            });
}

}
