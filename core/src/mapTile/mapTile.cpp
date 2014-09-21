#include "mapTile.h"

MapTile::MapTile(glm::ivec3 _tileCoord) : m_MercXYZ(_tileCoord) {
}

bool MapTile::setVBO(std::vector<float> _vboData) {
    return false;
}

glm::vec2 MapTile::GetBoundingBox() {
    return glm::vec2(0,0);
}

glm::vec3 MapTile::MercToPix() {
    return glm::vec3(0,0,0);
}

glm::vec2 MapTile::MercToLatLong() {
    return glm::vec2(0,0);
}
