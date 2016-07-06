#include "util/yamlHelper.h"

#include "log.h"
#include "csscolorparser.hpp"

namespace Tangram {

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
