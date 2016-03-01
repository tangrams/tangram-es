#pragma once

#include "util/variant.h"

#include "glm/glm.hpp"

#include <string>

namespace Tangram {

/* Style Block Uniform types */
using UniformValue = variant<none_type, bool, std::string, float, int, glm::vec2, glm::vec3,
      glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

class UniformLocation {
    std::string name;

public:
    UniformLocation() {}
    UniformLocation(const std::string& _name) : name(_name) {}
    const std::string& getName() const { return name; }

    mutable int location = -1;
    mutable int generation = -1;
};

}
