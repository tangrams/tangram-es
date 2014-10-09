#include "geometry.h"
#include "tesselator.h"

static TESStesselator* geometry::tesselator = nullptr;

void geometry::init() {
    // might not need this, can just check if tesselator is valid in each context that uses it
    if (tesselator == nullptr) {
        tessNewTess(tesselator);
    }
}

void geometry::cleanup() {
    if (tesselator != nullptr) {
        tessDeleteTess(tesselator);
    }
}

void geometry::buildPolygon(const std::vector<glm::vec3>& _pointsIn, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut) {

}

void geometry::buildPolygonExtrusion(const std::vector<glm::vec3>& _pointsIn, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut) {

}

void geometry::buildPolyLine(const std::vector<glm::vec3>& _pointsIn, float width, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut) {

}

void geometry::buildQuadAtPoint(const glm::vec3& _pointIn, const glm::vec3& _normal, float width, float height, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut) {

}
