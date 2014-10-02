#pragma once

#include <unordered_map>
#include <memory>

#define GLM_FORCE_RADIANS
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "util/vboMesh.h"
#include "util/projection.h"

class Style;

/* An immutable identifier for a map tile 
 * 
 * Contains the x, y, and z indices of a tile in a quad tree; TileIDs are arbitrarily but strictly ordered
 */
struct TileID {
    
    TileID(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {};

    bool operator< (const TileID& _rhs) const { return (x < _rhs.x || (y < _rhs.y || z < _rhs.z)); }
    bool operator> (const TileID& _rhs) const { return _rhs < const_cast<TileID&>(*this); }
    bool operator<=(const TileID& _rhs) const { return !(*this > _rhs); }
    bool operator>=(const TileID& _rhs) const { return !(*this < _rhs); }
    bool operator==(const TileID& _rhs) const { return x == _rhs.x && y == _rhs.y && z == _rhs.z; }

    const int x;
    const int y;
    const int z;

};

/* Tile of vector map data
 * 
 * MapTile represents a fixed area of a map at a fixed zoom level; It contains its position within a quadtree of
 * tiles and its location in projected global space; It stores drawable geometry of the map features in its area
 */
class MapTile {

public:
    
    MapTile(TileID _id, const MapProjection& _projection);

    virtual ~MapTile();

    /* Returns the immutable <TileID> of this tile */
    const TileID& getID() const { return m_id; };

    /* Returns the lower left corner of the tile area in projection units */
    const glm::dvec2& getOrigin() const { return m_tileOrigin; };

    /* Adds drawable geometry to the tile and associates it with a <Style> 
     * 
     * Use std::move to pass in the mesh by move semantics; Geometry in the mesh must have coordinates relative to
     * the tile origin.
     */
    void addGeometry(const Style& _style, std::unique_ptr<VboMesh> _mesh);

    /* Draws the geometry associated with the provided <Style> and view-projection matrix */
    void draw(const Style& _style, const glm::dmat4& _viewProjMatrix);

private:

    TileID m_id;

    glm::dvec2 m_tileOrigin; // Lower-left corner of the tile in 2D projection space in meters (e.g. mercator meters)

    glm::dmat4 m_modelMatrix; // Translation matrix from world origin to tile origin

    std::unordered_map<std::string, std::unique_ptr<VboMesh>> m_geometry;

};
