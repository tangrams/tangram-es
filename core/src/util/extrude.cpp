#include "util/extrude.h"

#include "util/yamlUtil.h"
#include <cmath>

namespace Tangram {

Extrude parseExtrudeNode(const YAML::Node& node) {
    // Values specified from the stylesheet are assumed to be meters with no unit suffix
    float first = 0, second = 0;

    bool extrudeBoolean = false;
    if (YamlUtil::getBool(node, extrudeBoolean)) {
        if (extrudeBoolean) {
            // "true" means use default properties for both heights, we indicate this with NANs
            return Extrude(NAN, NAN);
        }
        // "false" means perform no extrusion
        return Extrude(0, 0);
    }

    if (node.IsSequence() && node.size() >= 2) {
        if (YamlUtil::getFloat(node[0], first) && YamlUtil::getFloat(node[1], second)) {
            // Got two numbers, so return an extrusion from first to second
            return Extrude(first, second);
        }
    }

    if (YamlUtil::getFloat(node, first)) {
        // No second number, so return an extrusion from 0 to the first number
        return Extrude(0, first);
    }

    // No numbers found, return zero extrusion
    return Extrude(0, 0);
}

float getLowerExtrudeMeters(const Extrude& _extrude, const Properties& _props) {

    const static std::string key_min_height("min_height");

    double lower = 0;

    if (std::isnan(_extrude[0])) {
        // A NAN indicates that the default property should be used for this height
        _props.getNumber(key_min_height, lower);
    } else {
        lower = _extrude[0];
    }

    return lower;

}

float getUpperExtrudeMeters(const Extrude& _extrude, const Properties& _props) {

    const static std::string key_height("height");

    double upper = 0;

    if (std::isnan(_extrude[1])) {
        // A NAN indicates that the default property should be used for this height
        _props.getNumber(key_height, upper);
    } else {
        upper = _extrude[1];
    }

    return upper;

}

}
