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

public:

    SceneLayer(std::string _name, Filter _filter,
               std::vector<DrawRuleData> _rules,
               std::vector<SceneLayer> _sublayers);

    const auto& name() const { return m_name; }
    const auto& filter() const { return m_filter; }
    const auto& rules() const { return m_rules; }
    const auto& sublayers() const { return m_sublayers; }
};

}

