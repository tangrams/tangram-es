#pragma once

#include "util/shaderProgram.h"
#include "util/vertexLayout.h"
#include "glm/glm.hpp"
#include "glm/mat4x4.hpp"

namespace Primitives {
    
    void init(glm::vec2 _resolution);

    void drawLine(const glm::vec2& _origin, const glm::vec2& _destination);
}
