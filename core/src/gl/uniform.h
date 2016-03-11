#pragma once

#include "util/variant.h"

#include "glm/glm.hpp"

#include <string>
#include <vector>

namespace Tangram {

class ShaderProgram;

struct UniformTextureArray {
    std::vector<std::string> names;
    std::vector<int> slots;

    inline bool operator==(UniformTextureArray& uta) {
        return uta.slots == slots;
    };
};

using UniformArray = std::vector<float>;

/* Style Block Uniform types */
using UniformValue = variant<none_type, bool, std::string, float, int, glm::vec2, glm::vec3,
      glm::vec4, glm::mat2, glm::mat3, glm::mat4, UniformArray, UniformTextureArray>;


class UniformLocation {

public:
    UniformLocation(const std::string& _name) : name(_name) {}

private:
    const std::string name;

    mutable int location = -1;
    mutable int generation = -1;

    friend class ShaderProgram;
};

}
