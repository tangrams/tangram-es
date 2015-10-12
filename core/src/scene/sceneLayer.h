#pragma once

#include "data/filters.h"
#include "scene/drawRule.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Tangram {

struct DrawRule;
struct Feature;

class SceneLayer {

    Filter m_filter;
    std::string m_name;
    std::vector<DrawRule> m_rules;
    std::vector<SceneLayer> m_sublayers;

public:

    SceneLayer(std::string _name, Filter _filter,
        std::vector<DrawRule> _rules, std::vector<SceneLayer> _sublayers);

    const auto& name() const { return m_name; }
    const auto& filter() const { return m_filter; }
    const auto& rules() const { return m_rules; }
    const auto& sublayers() const { return m_sublayers; }

    // Match and combine draw rules that apply to the given Feature in the given Context
    std::vector<DrawRule> match(const Feature& _feat, const StyleContext& _ctx) const;

};

}

