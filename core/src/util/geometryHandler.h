#pragma once

#include <vector>

#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "tesselator.h"

#include "platform.h"

namespace GeometryHandler {

    static TESStesselator* tesselator = nullptr;

    void init();

    void cleanup();

    /*
     * _pointsIn: set of rings from jsonCoordinates or other formats.
     * _pointsOut: a vector of tesselated vertex coordinates
     * _normalsOut: normals if generated from tess, else default
     * _indicesOut: set of indices generated from the tesselator
     */
    void buildPolygon(const std::vector<glm::vec3>& _pointsIn, const std::vector<int>& _ringSizes, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<GLushort>& _indicesOut);

    void buildPolygonExtrusion(const std::vector<glm::vec3>& _pointsIn, const std::vector<int>& _ringSizes, const float& _minFeatureHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<GLushort>& _indicesOut);

    void buildPolyLine(const std::vector<glm::vec3>& _pointsIn, float width, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut);

    void buildQuadAtPoint(const glm::vec3& _pointIn, const glm::vec3& _normal, float width, float height, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut);
    
}
