// Helper Functions for parsing YAML nodes
// NOTE: To be used in multiple YAML parsing modules once SceneLoader aptly modularized

#pragma once

#include "yaml-cpp/yaml.h"

namespace Tangram {

// A YamlPath encodes the location of a node in a yaml document in a string,
// e.g. "lorem.ipsum#0" identifies root["lorem"]["ipsum"][0]
struct YamlPath {
    // Follow this path from a root node and set 'out' to the result.
    // Returns true if the path exists up to the final token (i.e. the output
    // may be a new node), otherwise returns false and leaves 'out' unchanged.
    static bool get(const YAML::Node& root, const std::string& path, YAML::Node& out);
};

}
