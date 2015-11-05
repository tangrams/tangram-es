#pragma once

#include "util/variant.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include <string>

namespace Tangram {

/* Style Block Uniform types */
using UniformValue = variant<none_type, bool, std::string, float, glm::vec2, glm::vec3, glm::vec4>;

}
