#pragma once

#include "glm/vec2.hpp"

namespace Tangram {

class RenderState;
class Texture;

namespace Primitives {

void init();
void deinit();

/* Setup the debug resolution size */
void setResolution(RenderState& rs, float _width, float _height);

/* Sets the current primitive color */
void setColor(RenderState& rs, unsigned int _color);

/* Draws a line from _origin to _destination for the screen resolution _resolution */
void drawLine(RenderState& rs, const glm::vec2& _origin, const glm::vec2& _destination);

/* Draws a rect from _origin to _destination for the screen resolution _resolution */
void drawRect(RenderState& rs, const glm::vec2& _origin, const glm::vec2& _destination);

/* Draws a polyon of containing _n points in screen space for the screen resolution _resolution */
void drawPoly(RenderState& rs, const glm::vec2* _polygon, size_t _n);

void drawTexture(RenderState& rs, Texture& _tex, glm::vec2 _pos, glm::vec2 _dim);

}

}
