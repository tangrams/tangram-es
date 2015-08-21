#pragma once

#include "glm/vec2.hpp"

namespace Tangram {

namespace Primitives {

/* Setup the debug resolution size */
void setDebugResolution(float _width, float _height);

/* Sets the current primitive color */
void setDebugColor(unsigned int _color);

/* Draws a line from _origin to _destination for the screen resolution _resolution */
void drawLine(const glm::vec2& _origin, const glm::vec2& _destination);

/* Draws a rect from _origin to _destination for the screen resolution _resolution */
void drawRect(const glm::vec2& _origin, const glm::vec2& _destination);

/* Draws a polyon of containing _n points in screen space for the screen resolution _resolution */
void drawPoly(const glm::vec2* _polygon, size_t _n);

}

}
