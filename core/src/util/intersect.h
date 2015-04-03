#pragma once

#include "glm/glm.hpp"

/* Returns true iff the axis-aligned bounding box aabb intersects the trapezoid ABCD 
 * @_aabb the axis-aligned bounding box, given as [xmin, ymin, xmax, ymax]
 * @_A @_B @_C @_D the vertices of the trapezoid, where segments AB and CD are parallel
 */
bool AABBIntersectsTrapezoid(const glm::vec4& _aabb, const glm::vec2& _A, const glm::vec2& _B, const glm::vec2& _C, const glm::vec2& _D);
