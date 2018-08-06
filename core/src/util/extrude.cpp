#include "util/extrude.h"

#include "util/floatFormatter.h"

#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/convert.h"
#include <cmath>
#include <cstdlib>

namespace Tangram {

Extrude parseExtrudeString(const std::string& _str) {

    // Values specified from the stylesheet are assumed to be meters with no unit suffix
    float first = 0, second = 0;

    if (_str == "true") {
        // "true" means use default properties for both heights, we indicate this with NANs
        return Extrude(NAN, NAN);
    }

    if (_str == "false") {
        // "false" means perform no extrusion
        return Extrude(0, 0);
    }

    // Parse the first of two possible numbers
    const char* str = _str.c_str();

    // Get a float if possible & advance pos to the end of the number
    const int length = _str.length();
    int count = 0;
    int offset = 0;

    first = ff::stof(str, length, &count);
    if (count == 0) {
        // No numbers found, return zero extrusion
        return Extrude(0, 0);
    }

    // Advance and skip the delimiter character
    offset += count;
    if (length - offset <= 0) {
        return Extrude(0, first);
    }
    while (str[offset] == ' ') { offset++; }
    if (str[offset++] != ',') {
        return Extrude(0, first);
    }

    // Get a float if possible & advance pos to the end of the number
    second = ff::stof(str + offset, length - offset, &count);
    if (count == 0) {
        // No second number, so return an extrusion from 0 to the first number
        return Extrude(0, first);
    }

    // Got two numbers, so return an extrusion from first to second
    return Extrude(first, second);

}

Extrude parseExtrudeNode(const YAML::Node& node) {
    // Values specified from the stylesheet are assumed to be meters with no unit suffix
    float first = 0, second = 0;

    bool extrudeBoolean = false;
    if (YAML::convert<bool>::decode(node, extrudeBoolean)) {
        if (extrudeBoolean) {
            // "true" means use default properties for both heights, we indicate this with NANs
            return Extrude(NAN, NAN);
        }
        // "false" means perform no extrusion
        return Extrude(0, 0);
    }

    if (YAML::convert<float>::decode(node, first)) {
        // No second number, so return an extrusion from 0 to the first number
        return Extrude(0, first);
    }

    if (node.IsSequence() && node.size() >= 2) {
        if (YAML::convert<float>::decode(node[0], first) && YAML::convert<float>::decode(node[1], second)) {
            // Got two numbers, so return an extrusion from first to second
            return Extrude(first, second);
        }
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
