#pragma once

#include "tangram.h"
#include "util/shaderProgram.h"
#include "util/vertexLayout.h"
#include "glm/glm.hpp"
#include "glm/mat4x4.hpp"
#include "debug.h"

namespace Primitives {
    
    /*
     * Draws a line from _origin to _destination for the screen resolution _resolution
     */
    void drawLine(const glm::vec2& _origin, const glm::vec2& _destination, glm::vec2 _resolution);
    
    /*
     * Draws a rect from _origin to _destination for the screen resolution _resolution
     */
    void drawRect(const glm::vec2& _origin, const glm::vec2& _destination, glm::vec2 _resolution);
    
    /*
     * Draws a polyon of containing _n points in screen space for the screen resolution _resolution
     */
    void drawPoly(const glm::vec2* _polygon, size_t _n, glm::vec2 _resolution);
    
}
