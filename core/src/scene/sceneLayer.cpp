#include "scene/sceneLayer.h"

#include <type_traits>

namespace Tangram {

static_assert(std::is_move_constructible<SceneLayer>::value, "check");

SceneLayer::SceneLayer(std::string name, Filter filter,
                       std::vector<DrawRuleData> rules,
                       std::vector<SceneLayer> sublayers,
                       Options options) :
    m_filter(std::move(filter)),
    m_name(std::move(name)),
    m_rules(std::move(rules)),
    m_sublayers(std::move(sublayers)),
    m_options(options) {

    // Sort sublayers for precedence in matching operations. If multiple values for a parameter are assigned to the same
    // draw group at the same layer depth, then the value that comes *first* in the layer list will be the final value.
    std::sort(m_sublayers.begin(), m_sublayers.end(),
              [](const SceneLayer& a, const SceneLayer& b) {
                  if (a.exclusive() != b.exclusive()) {
                      // Exclusive layers always precede non-exclusive layers.
                      return a.exclusive();
                  } else if (a.priority() != b.priority()) {
                      // Layers whose priority value is closer to -infinity take precedence.
                      return a.priority() < b.priority();
                  }
                  // If priority and exclusivity are the same for two layers, precedence is determined by reverse
                  // alphabetical ordering of their names (which must be unique among sibling layers). That is, if there
                  // are two sibling layers with the same exclusivity and priority named 'a' and 'b', then 'b' will come
                  // first.
                  return a.name() > b.name();
              });
}

}
