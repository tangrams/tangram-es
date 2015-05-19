#pragma once

#include "glm/glm.hpp"
#include "simpleMesh.h"
#include "rectangle.h"

SimpleMesh rect (float _x, float _y, float _w, float _h);
SimpleMesh rect (const Rectangle &_rect);

SimpleMesh rectBorders(const Rectangle &_rect);
SimpleMesh rectCorners(const Rectangle &_rect, float _width = 4.);

SimpleMesh cross(const glm::vec3 &_pos, float _width);
void drawCross(const glm::vec3 &_pos, const float &_width = 3.5);

SimpleMesh line (const glm::vec3 &_a, const glm::vec3 &_b);
SimpleMesh polyline (const std::vector<glm::vec3> &_pts );
