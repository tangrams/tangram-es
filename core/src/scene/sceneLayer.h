#pragma once

#include "scene/drawRule.h"
#include "scene/filters.h"

#include <string>
#include <vector>

namespace Tangram {

class SceneLayer {

public:

    struct Options {
        Options() = default;
        int priority = std::numeric_limits<int>::max();
        bool enabled = true;
        bool exclusive = false;
    };

    SceneLayer(std::string name, Filter filter,
               std::vector<DrawRuleData> rules,
               std::vector<SceneLayer> sublayers,
               Options options);

    const auto& name() const { return m_name; }
    const auto& filter() const { return m_filter; }
    const auto& rules() const { return m_rules; }
    const auto& sublayers() const { return m_sublayers; }
    auto priority() const { return m_options.priority; }
    auto enabled() const { return m_options.enabled; }
    auto exclusive() const { return m_options.exclusive; }

private:

    Filter m_filter;
    std::string m_name;
    std::vector<DrawRuleData> m_rules;
    std::vector<SceneLayer> m_sublayers;
    Options m_options;
};

}

