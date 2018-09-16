//
// Created by Matt Blair on 9/15/18.
//

#include "yamlUtil.h"
#include "log.h"
#include "util/floatFormatter.h"
#include "csscolorparser.hpp"

namespace Tangram {
namespace YamlUtil {

glm::vec4 getColorAsVec4(const YAML::Node& node) {
    double val;
    if (getDouble(node, val, false)) {
        return glm::vec4(val, val, val, 1.0);
    }
    if (node.IsSequence()) {
        glm::vec4 vec;
        if (parseVec(node, vec)) {
            return vec;
        }
    }
    if (node.IsScalar()) {
        auto c = CSSColorParser::parse(node.Scalar());
        return glm::vec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a);
    }
    return glm::vec4();
}

std::string parseSequence(const YAML::Node& node) {
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


bool getInt(const YAML::Node& node, int& result, bool allowTrailingJunk) {
    if (node.IsScalar()) {
        auto& str = node.Scalar();
        char* position = nullptr;
        constexpr int base = 10;
        long value = std::strtol(str.data(), &position, base);
        if (position == (str.data() + str.size()) || (position > str.data() && allowTrailingJunk)) {
            result = static_cast<int>(value);
            return true;
        }
    }
    return false;
}

int getIntOrDefault(const YAML::Node& node, int defaultValue, bool allowTrailingJunk) {
    getInt(node, defaultValue, allowTrailingJunk);
    return defaultValue;
}

bool getFloat(const YAML::Node& node, float& result, bool allowTrailingJunk) {
    double doubleValue;
    if (getDouble(node, doubleValue, allowTrailingJunk)) {
        result = static_cast<float>(doubleValue);
        return true;
    }
    return false;
}

float getFloatOrDefault(const YAML::Node& node, float defaultValue, bool allowTrailingJunk) {
    getFloat(node, defaultValue, allowTrailingJunk);
    return defaultValue;
}

bool getDouble(const YAML::Node& node, double& result, bool allowTrailingJunk) {
    if (node.IsScalar()) {
        const std::string& scalar = node.Scalar();
        int count = 0;
        result = ff::stod(scalar.data(), static_cast<int>(scalar.size()), &count);
        if (count == static_cast<int>(scalar.size()) || (count > 0 && allowTrailingJunk)) {
            return true;
        }
    }
    return false;
}

double getDoubleOrDefault(const YAML::Node& node, double defaultValue, bool allowTrailingJunk) {
    getDouble(node, defaultValue, allowTrailingJunk);
    return defaultValue;
}

bool getBool(const YAML::Node& node, bool& result) {
    return YAML::convert<bool>::decode(node, result);
}

bool getBoolOrDefault(const YAML::Node& node, bool defaultValue) {
    getBool(node, defaultValue);
    return defaultValue;
}

} // namespace YamlUtil
} // namespace Tangram
