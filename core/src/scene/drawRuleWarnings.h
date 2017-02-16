#pragma once

#if defined(TANGRAM_WARN_ON_RULE_CONFLICT)

#include <mutex>
#include <set>

namespace Tangram {

static std::set<std::string> log;
static std::mutex logMutex;

void evalConflict(const DrawRule& rule, const DrawRuleData& data, const SceneLayer& layer) {

    for (const auto& param : data.parameters) {

        auto k = static_cast<uint8_t>(param.key);

        if (rule.depths[k] == layer.depth()) {

            std::lock_guard<std::mutex> lock(logMutex);

            std::string logString = "Draw parameter '" + StyleParam::keyName(param.key) + "' in rule '" +
                data.name + "' in layer '" + layer.name() + "' conflicts with layer '" + rule.layers[k] + "'";

            if (log.insert(logString).second) {
                LOGW("%s", logString.c_str());
            }
        }

    }

}

}

#else

namespace Tangram {

void evalConflict(const DrawRule& rule, const DrawRuleData& data, const SceneLayer& layer) {}

}

#endif
