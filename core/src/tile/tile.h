#pragma once

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "tileID.h"
#include "util/fastmap.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Tangram {

class DataSource;
class MapProjection;
class Scene;
class Style;
class VboMesh;
class View;
class StyleContext;

/* Tile of vector map data
 *
 * Tile represents a fixed area of a map at a fixed zoom level; It contains its
 * position within a quadtree of tiles and its location in projected global
 * space; It stores drawable geometry of the map features in its area
 */
class Tile {

public:

    Tile(TileID _id, const MapProjection& _projection, const DataSource* _source = nullptr);


    virtual ~Tile();

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

    const glm::mat4& getModelMatrix() const { return m_modelMatrix; }

    // Exposing for labelsTest
    void initGeometry(uint32_t _size);

    std::unique_ptr<VboMesh>& getMesh(const Style& _style);

    /* Update the Tile considering the current view */
    void update(float _dt, const View& _view);

    /* Update tile origin based on wraping for this tile */
    void updateTileOrigin(const int _wrap);

    /* Draws the geometry associated with the provided <Style> and view-projection matrix */
    void draw(const Style& _style, const View& _view);

    void resetState();

    /* Get the sum in bytes of all <VboMesh>es */
    size_t getMemoryUsage() const;

    int64_t sourceGeneration() const { return m_sourceGeneration; }

    int32_t sourceID() const { return m_sourceId; }

    bool isProxy() const { return m_proxyState; }

    void setProxyState(bool isProxy) { m_proxyState = isProxy; }

private:

    const TileID m_id;

    const MapProjection* m_projection = nullptr;

    float m_scale = 1;

    float m_inverseScale = 1;

    /* ID of the DataSource */
    const int32_t m_sourceId;

    /* State of the DataSource for which this tile was created */
    const int64_t m_sourceGeneration;

    bool m_proxyState = false;

    glm::dvec2 m_tileOrigin; // South-West corner of the tile in 2D projection space in meters (e.g. mercator meters)

    glm::mat4 m_modelMatrix; // Matrix relating tile-local coordinates to global projection space coordinates;
    // Note that this matrix does not contain the relative translation from the global origin to the tile origin.
    // Distances from the global origin are too large to represent precisely in 32-bit floats, so we only apply the
    // relative translation from the view origin to the model origin immediately before drawing the tile.

    // Map of <Style>s and their associated <VboMesh>es
    fastmap<std::string, std::unique_ptr<VboMesh>> m_geometry;
    //std::vector<std::unique_ptr<VboMesh>> m_geometry;

    mutable size_t m_memoryUsage = 0;
};

}
