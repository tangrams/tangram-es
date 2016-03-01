// Helper Functions for parsing YAML nodes
// NOTE: To be used in multiple YAML parsing modules once SceneLoader aptly modularized

#pragma once

#include "yaml-cpp/yaml.h"
#include "glm/vec4.hpp"

using YAML::Node;
using YAML::BadConversion;

namespace Tangram {

template<typename T>
inline T parseVec(const Node& node) {
    T vec;
    int i = 0;
    for (const auto& nodeVal : node) {
        if (i < vec.length()) {
            vec[i++] = nodeVal.as<double>();
        } else {
            break;
        }
    }
    return vec;
}

glm::vec4 getColorAsVec4(const Node& node);

std::string parseSequence(const Node& node);

bool getDouble(const Node& node, double& value, const char* name = nullptr);

bool getBool(const Node& node, bool& value, const char* name = nullptr);

}
