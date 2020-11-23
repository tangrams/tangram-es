#pragma once

#include "glm/vec3.hpp"
#include <functional>

namespace Tangram {

class View;

std::function<glm::dvec3(float)> getFlyToFunction(const View& view, glm::dvec3 start, glm::dvec3 end, double& distance);

} // namespace Tangram
