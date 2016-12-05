// Helper Functions for parsing YAML nodes
// NOTE: To be used in multiple YAML parsing modules once SceneLoader aptly modularized

#pragma once

#include "yaml-cpp/yaml.h"
#include "glm/vec4.hpp"

using YAML::Node;
using YAML::BadConversion;

namespace Tangram {

// A YamlPath encodes the location of a node in a yaml document in a string,
// e.g. "lorem.ipsum#0" identifies root["lorem"]["ipsum"][0]
struct YamlPath {
    YamlPath();
    YamlPath(const std::string& path);
    YamlPath add(int index);
    YamlPath add(const std::string& key);
    YAML::Node get(YAML::Node root);
    std::string codedPath;
};

struct YamlPathBuffer {

    struct PathElement {
        size_t index;
        const std::string* key;
        PathElement(size_t index, const std::string* key) : index(index), key(key) {}
    };

    std::vector<PathElement> m_path;

    void pushMap(const std::string* _p) { m_path.emplace_back(0, _p);}
    void pushSequence() { m_path.emplace_back(0, nullptr); }
    void increment() { m_path.back().index++; }
    void pop() { m_path.pop_back(); }
    YamlPath toYamlPath();
};

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

bool getDouble(const Node& node, double& value);
bool getDouble(const Node& node, double& value, const char* name);

bool getBool(const Node& node, bool& value, const char* name = nullptr);

}
