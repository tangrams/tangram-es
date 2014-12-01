#pragma once

#include <vector>

#include "tileData.h"
#include "platform.h"

namespace GeometryHandler {

    typedef unsigned short ushort;

    void buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<ushort>& _indicesOut);
    void buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<ushort>& _indicesOut, std::vector<glm::vec2>& _texcoordsOut);

    void buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<ushort>& _indicesOut);
    void buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<ushort>& _indicesOut, std::vector<glm::vec2>& _texcoordsOut);

    void buildPolyLine(const Line& _line, float _halfWidth, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut);
    void buildPolyLine(const Line& _line, float _halfWidth, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut, std::vector<glm::vec2>& _texcoordsOut);
    
    void buildScalablePolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<ushort>& _indicesOut);
    void buildScalablePolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<ushort>& _indicesOut, std::vector<glm::vec2>& _texcoordsOut);
    
    void buildQuadAtPoint(const Point& _pointIn, const glm::vec3& _normal, float width, float height, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut);
    
}
