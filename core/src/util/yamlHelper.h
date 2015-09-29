// Helper Functions for parsing YAML nodes
// NOTE: To be used in multiple YAML parsing modules once SceneLoader aptly modularized

#pragma once

using YAML::Node;
using YAML::BadConversion;

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

template<typename T>
T parseVec(const Node& node) {
    T vec;
    int i = 0;
    for (const auto& nodeVal : node) {
        if (i < vec.length()) {
            vec[i++] = nodeVal.as<float>();
        } else {
            break;
        }
    }
    return vec;
}

}
