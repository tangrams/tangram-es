#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "util/vboMesh.h"
#include "util/projection.h"
#include "style/style.h"
#include <unordered_map>

class MapTile {

public:
    
    MapTile(glm::ivec3 _address, const Projection& _projection);

    virtual ~MapTile();

    /*
     * compareTileID provides a strict, weak ordering on 3-component tile IDs; returns true if the first ID precedes the second
     */
    static struct tileIDComparator {
        bool operator() (const glm::ivec3& _a, const glm::ivec3& _b) const {
            if ( _a.x != _b.x) {
                return _a.x < _b.x;
            } else if (_a.y != _b.y) {
                return _a.y < _b.y;
            } else {
                return _a.z < _b.z;
            }
        }
    };

    const glm::ivec3& getAddress() const { return m_address; };

    const glm::vec2& getTileOrigin() const { return m_tileOrigin; };

    void addGeometry(const Style& _style, unique_ptr<VboMesh>&& _mesh);

    void draw(const Style& _style, const glm::mat4& _viewProjMatrix);

private:

    glm::ivec3 m_address;

    glm::vec2 m_tileOrigin; // Lower-left corner of the tile in 2D projection space in meters (e.g. mercator meters)

    glm::mat4 m_modelMatrix; // Translation matrix from world origin to tile origin

    std::unordered_map<Style, std::unique_ptr<VboMesh>> m_geometry;

};
