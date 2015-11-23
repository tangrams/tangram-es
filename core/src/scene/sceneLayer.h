#pragma once

#include "scene/drawRule.h"
#include "scene/filters.h"
#include "scene/styleParam.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Tangram {

struct Feature;

struct StaticDrawRule {
    std::string name;
    int id;
    std::vector<StyleParam> parameters;

    StaticDrawRule(std::string _name, int _id,
                   const std::vector<StyleParam>& _parameters);

    std::string toString() const;

    bool operator<(const StaticDrawRule& _rhs) const {
        return id < _rhs.id;
    }
};

class SceneLayer {

    Filter m_filter;
    std::string m_name;
    std::vector<StaticDrawRule> m_rules;
    std::vector<SceneLayer> m_sublayers;

public:

    SceneLayer(std::string _name, Filter _filter,
               std::vector<StaticDrawRule> _rules,
               std::vector<SceneLayer> _sublayers);

    const auto& name() const { return m_name; }
    const auto& filter() const { return m_filter; }
    const auto& rules() const { return m_rules; }
    const auto& sublayers() const { return m_sublayers; }
};

}

