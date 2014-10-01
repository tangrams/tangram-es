#pragma once

#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "util/vboMesh.h"
#include "util/projection.h"
#include "style/style.h"
#include <unordered_map>

class MapTile {

public:
    
    MapTile(tileID _id, const Projection& _projection);

    virtual ~MapTile();

    struct tileID {
        
        tileID(int _x, int _y, int _z) : x(_x) y(_y) z(_z) {};

        const int x;
        const int y;
        const int z;

    };

    /*
     * tileIDComparator provides a strict, weak ordering on 3-component tile IDs; returns true if the first ID precedes the second
     */
    static struct tileIDComparator {
        bool operator() (const tileID& _a, const tileID& _b) const {
            if ( _a.x != _b.x) {
                return _a.x < _b.x;
            } else if (_a.y != _b.y) {
                return _a.y < _b.y;
            } else {
                return _a.z < _b.z;
            }
        }
    };

    const tileID& getID() const { return m_id; };

    const glm::dvec2& getTileOrigin() const { return m_tileOrigin; };

    void addGeometry(const Style& _style, unique_ptr<VboMesh>&& _mesh);

    void draw(const Style& _style, const glm::dmat4& _viewProjMatrix);

private:

    tileID m_id;

    glm::dvec2 m_tileOrigin; // Lower-left corner of the tile in 2D projection space in meters (e.g. mercator meters)

    glm::dmat4 m_modelMatrix; // Translation matrix from world origin to tile origin

    std::unordered_map<std::string, std::unique_ptr<VboMesh>> m_geometry;

};
