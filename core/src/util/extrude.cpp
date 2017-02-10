#include "util/extrude.h"

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
    const char* pos = _str.c_str();
    char* end = nullptr;

    // Get a float if possible & advance pos to the end of the number
    first = std::strtof(pos, &end);
    if (pos == end) {
        // No numbers found, return zero extrusion
        return Extrude(0, 0);
    }

    // Advance and skip the delimiter character
    pos = end + 1;

    // Get a float if possible & advance pos to the end of the number
    second = std::strtof(pos, &end);
    if (pos == end) {
        // No second number, so return an extrusion from 0 to the first number
        return Extrude(0, first);
    }

    // Got two numbers, so return an extrusion from first to second
    return Extrude(first, second);

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
