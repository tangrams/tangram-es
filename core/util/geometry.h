#pragma once

#include <vector>

#include "glm/fwd.hpp"
#include "glm/glm.hpp"

namespace geometry {

    void init();

    void cleanup();

    void buildPolygon(const std::vector<glm::vec3>& _pointsIn, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut);

    void buildPolygonExtrusion(const std::vector<glm::vec3>& _pointsIn, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut);

    void buildPolyLine(const std::vector<glm::vec3>& _pointsIn, float width, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut);

    void buildQuadAtPoint(const glm::vec3& _pointIn, const glm::vec3& _normal, float width, float height, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut);
    
}