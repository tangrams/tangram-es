#include "util/yamlHelper.h"

#include "log.h"
#include "csscolorparser.hpp"

#define MAP_DELIM '.'
#define SEQ_DELIM '#'

namespace Tangram {

YamlPath::YamlPath() {}

YamlPath::YamlPath(const std::string& path)
    : codedPath(path) {}

YamlPath YamlPath::add(int index) {
    return YamlPath(codedPath + SEQ_DELIM + std::to_string(index));
}

YamlPath YamlPath::add(const std::string& key) {
    if (codedPath.empty()) { return YamlPath(key); }
    return YamlPath(codedPath + MAP_DELIM + key);
}

YAML::Node YamlPath::get(YAML::Node node) {
    size_t beginToken = 0, endToken = 0;
    auto delimiter = MAP_DELIM; // First token must be a map key.
    while (endToken < codedPath.size()) {
        beginToken = endToken;
        endToken = codedPath.size();
        endToken = std::min(endToken, codedPath.find(SEQ_DELIM, beginToken));
        endToken = std::min(endToken, codedPath.find(MAP_DELIM, beginToken));
        if (delimiter == SEQ_DELIM) {
            int index = std::stoi(&codedPath[beginToken]);
            node.reset(node[index]);
        } else if (delimiter == MAP_DELIM) {
            auto key = codedPath.substr(beginToken, endToken - beginToken);
            node.reset(node[key]);
        } else {
            return Node(); // Path is malformed, return null node.
        }
        delimiter = codedPath[endToken]; // Get next character as the delimiter.
        ++endToken; // Move past the delimiter.
    }
    return node;
}

glm::vec4 getColorAsVec4(const Node& node) {
    double val;
    if (getDouble(node, val)) {
        return glm::vec4(val, val, val, 1.0);
    }
    if (node.IsSequence()) {
        return parseVec<glm::vec4>(node);
    }
    if (node.IsScalar()) {
        auto c = CSSColorParser::parse(node.Scalar());
        return glm::vec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a);
    }
    return glm::vec4();
}

std::string parseSequence(const Node& node) {
    std::stringstream sstream;
    for (const auto& val : node) {
        try {
            sstream << val.as<double>() << ",";
        } catch (const BadConversion& e) {
            try {
                sstream << val.as<std::string>() << ",";
            } catch (const BadConversion& e) {
                LOGE("Float or Unit expected for styleParam sequence value");
            }
        }
    }
    return sstream.str();
}

bool getDouble(const Node& node, double& value, const char* name) {
    try {
        value = node.as<double>();
        return true;
    } catch (const BadConversion& e) {}

    if (name) {
        LOGW("Expected a floating point value for '%s' property.:\n%s\n", name, Dump(node).c_str());
    }
    return false;
}

bool getBool(const Node& node, bool& value, const char* name) {
    try {
        value = node.as<bool>();
        return true;
    } catch (const BadConversion& e) {}

    if (name) {
        LOGW("Expected a boolean value for '%s' property.:\n%s\n", name, Dump(node).c_str());
    }
    return false;
}

}
