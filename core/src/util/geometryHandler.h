#pragma once

#include <vector>

#include "tileData.h"
#include "platform.h"

namespace GeometryHandler {

    typedef unsigned short ushort;

    void buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<ushort>& _indicesOut);

    void buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<ushort>& _indicesOut);

    void buildPolyLine(const Line& _line, float width, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut);

    void buildQuadAtPoint(const Point& _pointIn, const glm::vec3& _normal, float width, float height, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut);
    
}
