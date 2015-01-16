#pragma once

#include <unordered_map>
#include <memory>

#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"

#include "util/vboMesh.h"
#include "util/mapProjection.h"

class Style;
struct TileID;

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
    const TileID& getID() const { return m_id; }

    /* Returns the center of the tile area in projection units */
    const glm::dvec2& getOrigin() const { return m_tileOrigin; }
    
    /* Returns the map projection with which this tile interprets coordinates */
    const MapProjection* getProjection() const { return m_projection; }
    
    /* Returns the length of a side of this tile in projection units */
    float getScale() const { return m_scale; }
    
    /* Returns the reciprocal of <getScale()> */
    float getInverseScale() const { return m_inverseScale; }

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
    
    const MapProjection* m_projection = nullptr;
    
    float m_scale = 1;
    
    float m_inverseScale = 1;

    glm::dvec2 m_tileOrigin; // Center of the tile in 2D projection space in meters (e.g. mercator meters)

    glm::dmat4 m_modelMatrix; // Translation matrix from world origin to tile origin

    std::unordered_map<std::string, std::unique_ptr<VboMesh>> m_geometry; // Map of <Style>s and their associated <VboMesh>es

};

