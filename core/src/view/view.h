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

    View(float _width = 800, float _height = 600, ProjectionType _projType = ProjectionType::mercator);
    
    //Sets a new map projection with default tileSize
    void setMapProjection(ProjectionType _projType);
    //get the current mapProjection
    const MapProjection& getMapProjection();

    void setAspect(float _width, float _height);
    void setPosition(double _x, double _y);
    void setZoom(int _z);
    void translate(double _dx, double _dy);
    void zoom(int _dz);

    int getZoom() const { return m_zoom; };
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
    
    static const int s_maxZoom = 18;

private:

    std::unique_ptr<MapProjection> m_projection;
    bool m_dirty;
    std::set<TileID> m_visibleTiles;
    glm::dvec3 m_pos;
    glm::dmat4 m_view;
    glm::dmat4 m_proj;
    int m_zoom;
    float m_width;
    float m_height;
    double m_aspect;
};

