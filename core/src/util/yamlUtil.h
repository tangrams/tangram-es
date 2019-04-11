//
// Created by Matt Blair on 9/15/18.
//
#pragma once

#include "yaml-cpp/yaml.h"
#include "glm/vec4.hpp"

namespace Tangram {
namespace YamlUtil {

bool getInt(const YAML::Node& node, int& result, bool allowTrailingJunk = false);

int getIntOrDefault(const YAML::Node& node, int defaultValue, bool allowTrailingJunk = false);

bool getFloat(const YAML::Node& node, float& result, bool allowTrailingJunk = false);

float getFloatOrDefault(const YAML::Node& node, float defaultValue, bool allowTrailingJunk = false);

bool getDouble(const YAML::Node& node, double& result, bool allowTrailingJunk = false);

double getDoubleOrDefault(const YAML::Node& node, double defaultValue, bool allowTrailingJunk = false);

bool getBool(const YAML::Node& node, bool& result);

bool getBoolOrDefault(const YAML::Node& node, bool defaultValue);

template<typename T>
bool parseVec(const YAML::Node& sequence, T& result) {
    if (!sequence.IsSequence()) {
        return false;
    }
    int i = 0;
    double value;
    for (const auto& node : sequence) {
        if (i < result.length() && getDouble(node, value)) {
            result[i++] = value;
        } else {
            return false;
        }
    }
    return true;
}

glm::vec4 getColorAsVec4(const YAML::Node& node);

} // namespace YamlUtil
} // namespace Tangram
