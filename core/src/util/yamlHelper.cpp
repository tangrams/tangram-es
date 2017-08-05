#include "util/yamlHelper.h"

#include "util/floatFormatter.h"
#include "log.h"
#include "csscolorparser.hpp"

#include <cmath>

#define MAP_DELIM '.'
#define SEQ_DELIM '#'

namespace Tangram {

YamlPath YamlPathBuffer::toYamlPath() {
    size_t length = 0;

    for(auto& element : m_path) {
        if (element.key) {
            length += element.key->length()+1;
        } else {
            int c = 1;
            while (element.index >= pow(10, c)) { c += 1; }
            length += c + 1;
        }
    }
    std::string path;
    path.reserve(length);

    for(auto& element : m_path) {
        if (element.key) {
            if (!path.empty()) { path += '.'; }
            path += *element.key;
        } else {
            path += '#';
            path += std::to_string(element.index);
        }
    }
    return YamlPath(path);
}

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

bool YamlPath::get(YAML::Node root, YAML::Node& out) {
    size_t beginToken = 0, endToken = 0, pathSize = codedPath.size();
    auto delimiter = MAP_DELIM; // First token must be a map key.
    while (endToken < pathSize) {
        if (!root.IsDefined()) {
            return false; // A node before the end of the path was mising, quit!
        }
        beginToken = endToken;
        endToken = pathSize;
        endToken = std::min(endToken, codedPath.find(SEQ_DELIM, beginToken));
        endToken = std::min(endToken, codedPath.find(MAP_DELIM, beginToken));
        if (delimiter == SEQ_DELIM) {
            int index = std::stoi(&codedPath[beginToken]);
            if (root.IsSequence()) {
                root.reset(root[index]);
            } else {
                return false;
            }
        } else if (delimiter == MAP_DELIM) {
            auto key = codedPath.substr(beginToken, endToken - beginToken);
            if (root.IsMap()) {
                root.reset(root[key]);
            } else {
                return false;
            }
        } else {
            return false; // Path is malformed, return null node.
        }
        delimiter = codedPath[endToken]; // Get next character as the delimiter.
        ++endToken; // Move past the delimiter.
    }
    // Success! Assign the result.
    out.reset(root);
    return true;
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
    if (!node.IsSequence()) {
        LOGW("Expected a plain sequence: '%'", Dump(node).c_str());
        return "";
    }

    std::stringstream sstream;
    for (const auto& val : node) {
        if (!val.IsScalar()) {
            LOGW("Expected a plain sequence: '%'", Dump(node).c_str());
            return "";
        }
        sstream << val.Scalar() << ",";
    }
    return sstream.str();
}

bool getDouble(const Node& node, double& value) {

    if (node.IsScalar()) {
        const std::string& s = node.Scalar();
        int count;
        value = ff::stod(s.c_str(), s.length(), &count);
        if (count == int(s.length())) {
            return true;
        }
    }

    return false;
}

bool getDouble(const Node& node, double& value, const char* name) {

    if (getDouble(node, value)) { return true; }

    LOGW("Expected a floating point value for '%s' property.:\n%s\n", name, Dump(node).c_str());
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
