#pragma once

#include <functional>
#include "glm/glm.hpp"

namespace Tangram {

class View;

float getMinimumEnclosingZoom(double aLng, double aLat, double bLng, double bLat, const View& view, float buffer);

std::function<glm::dvec3(float)> getFlyToFunction(const View& view, glm::dvec3 start, glm::dvec3 end, double& distance);

} // namespace Tangram
