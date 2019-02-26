#pragma once

#include "scene/drawRule.h"
#include "scene/filters.h"
#include "scene/styleParam.h"

#include <string>
#include <vector>

namespace Tangram {

struct Feature;

class SceneLayer {

    Filter m_filter;
    std::string m_name;
    std::vector<DrawRuleData> m_rules;
    std::vector<SceneLayer> m_sublayers;
    size_t m_depth = 0;
    int m_priority = std::numeric_limits<int>::max();
    bool m_enabled = true;
    bool m_exclusive = false;
    mutable bool m_subLayersSorted = false;

public:

    SceneLayer(std::string _name, Filter _filter,
               std::vector<DrawRuleData> _rules,
               std::vector<SceneLayer> _sublayers,
               int _priority,
               bool _enabled,
               bool _exclusive);

    const auto& name() const { return m_name; }
    const auto& filter() const { return m_filter; }
    const auto& rules() const { return m_rules; }
    const auto& sublayers() const { return m_sublayers; }
    const auto& depth() const { return m_depth; }
    const auto& priority() const { return m_priority; }
    const auto& enabled() const { return m_enabled; }
    const auto& exclusive() const { return m_exclusive; }

    void setDepth(size_t _d);
    void sortSublayers();
};

}

