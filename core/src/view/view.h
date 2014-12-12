#pragma once

#include <vector>
#include <set>
#include <cmath>
#include <memory>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "util/mapProjection.h"
#include "util/tileID.h"

/* ViewModule
 * 1. Stores a representation of the current view into the map world
 * 2. Determines which tiles are visible in the current view
 * 3. Tracks changes in the view state to determine when new rendering is needed
 *
 * TODO: Make this into an interface for different implementations
 * For now, this is a simple implementation of the viewModule responsibilities
 * using a top-down axis-aligned orthographic view
*/

class View {

public:

    View(int _width = 800, int _height = 600, ProjectionType _projType = ProjectionType::mercator);
    
    //Sets a new map projection with default tileSize
    void setMapProjection(ProjectionType _projType);
    //get the current mapProjection
    const MapProjection& getMapProjection();

    void setSize(int _width, int _height);
    void setPosition(double _x, double _y);
    void setZoom(float _z);
    void translate(double _dx, double _dy);
    void zoom(float _dz);

    float getZoom() const { return m_zoom; };
    float getZoomState() const { return m_zoomState; };
    const glm::dvec3& getPosition() const { return m_pos; };
    const glm::dmat4& getViewMatrix() const { return m_view; };
    const glm::dmat4& getProjectionMatrix() const { return m_proj; };
    const glm::dmat4 getViewProjectionMatrix() const;

    glm::dmat2 getBoundsRect() const; // Returns a rectangle of the current view range as [[x_min, y_min], [x_max, y_max]]
    const std::set<TileID>& getVisibleTiles();
    bool viewChanged() const { return m_dirty; };

    virtual ~View() {
        m_visibleTiles.clear();
    }
    
    constexpr static const float s_maxZoom = 18.0;

private:

    std::unique_ptr<MapProjection> m_projection;
    bool m_dirty;
    std::set<TileID> m_visibleTiles;
    glm::dvec3 m_pos;
    glm::dmat4 m_view;
    glm::dmat4 m_proj;
    float m_zoom;
    float m_initZoom;
    /* +ve: zoom-in
     * -ve: zoom-out
     */
    bool m_zoomState;
    int m_vpWidth;
    int m_vpHeight;
    float m_width;
    float m_height;
    double m_aspect;
};

