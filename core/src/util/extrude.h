#pragma once

#include "data/properties.h"

#include "glm/vec2.hpp"
#include <string>

namespace Tangram {

// Extrude is a 2-element vector that follows specific conventions to encode extrusion options
using Extrude = glm::vec2;

// Returns an Extrude to represent the extrusion option specified in the string, one of:
// "true", "false", a single number, or a comma-separated pair of numbers
Extrude parseExtrudeString(const std::string& _str);

// Returns the lower or upper extrusion values for a given Extrude and set of feature properties
float getLowerExtrudeMeters(const Extrude& _extrude, const Properties& _props);
float getUpperExtrudeMeters(const Extrude& _extrude, const Properties& _props);

}
