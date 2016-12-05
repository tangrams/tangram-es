#pragma once

#include "yaml-cpp/node/node.h"

#include <string>

namespace Tangram {

struct YamlLoader {
    YamlLoader() = delete;

    // Throws YAML::ParserException
    static YAML::Node load(const char* bytes, size_t length);
    // Throws YAML::ParserException
    static YAML::Node load(const std::string& str) { return load(str.data(), str.size()); }
};

}
