#include "util/yamlHelper.h"

#include "platform.h"

namespace Tangram {

std::string parseSequence(const Node& node) {
    std::stringstream sstream;
    for (const auto& val : node) {
        try {
            sstream << val.as<float>() << ",";
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

bool getFloat(const Node& node, float& value, const char* name) {
    try {
        value = node.as<float>();
        return true;
    } catch (const BadConversion& e) {}

    if (name) {
        LOGW("Expected a float value for '%s' property.:\n%s\n", name, Dump(node).c_str());
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
