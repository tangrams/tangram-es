#include "view.h"
#include "util/tileID.h"
#include "platform.h"
#include "glm/gtx/string_cast.hpp"

const int View::s_maxZoom; // Create a stack reference to the static member variable

View::View(int _width, int _height, ProjectionType _projType) {
    //Set the map projection for the view module to use.
    setMapProjection(_projType);
    
    // Set up projection matrix based on input width and height with an arbitrary zoom
    setSize(_width, _height);
    setZoom(16); // Arbitrary zoom for testing

    // Set up view matrix
    m_pos = glm::dvec3(0, 0, 1000); // Start at 0 to begin
    glm::dvec3 direction = glm::dvec3(0, 0, -1); // Look straight down
    glm::dvec3 up = glm::dvec3(0, 1, 0); // Y-axis is 'up'
    m_view = glm::lookAt(m_pos, m_pos + direction, up);
}

void View::setMapProjection(ProjectionType _projType) {
    switch(_projType) {
        case ProjectionType::mercator:
            m_projection.reset(new MercatorProjection());
            break;
        default:
            logMsg("Error: not a valid map projection specified.\n Setting map projection to mercator by default");
            m_projection.reset(new MercatorProjection());
            break;
    }
    m_dirty = true;
}

const MapProjection& View::getMapProjection() {
    return *m_projection.get();
}

void View::setSize(int _width, int _height) {

    m_vpWidth = _width;
    m_vpHeight = _height;
    m_aspect = (float)_width / (float)_height;
    setZoom(m_zoom);
    m_dirty = true;

}

void View::setPosition(double _x, double _y) {

    translate(_x - m_pos.x, _y - m_pos.y);
    m_dirty = true;

}

void View::translate(double _dx, double _dy) {

    m_pos.x += _dx;
    m_pos.y += _dy;
    m_view = glm::lookAt(m_pos, m_pos + glm::dvec3(0, 0, -1), glm::dvec3(0, 1, 0));
    m_dirty = true;

}

void View::zoom(int _dz) {
    setZoom(m_zoom + _dz);
}

void View::setZoom(int _z) {

    // ensure zoom value is allowed
    glm::clamp(_z, 0, s_maxZoom);
    
    m_zoom = _z;
    
    // find dimensions of tiles in world space at new zoom level
    float tileSize = 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);
    
    // viewport height in world space is such that each tile is 256 px square in screen space
    m_height = (float)m_vpHeight / (float)256.0 * tileSize;
    m_width = m_height * m_aspect;
    
    // set vertical field-of-view to 90 deg
    double fovy = PI * 0.5;
    
    // set camera z to produce desired viewable area
    m_pos.z = m_height * 0.5 / tan(fovy * 0.5);
    
    // set near clipping distance as a function of camera z
    double near = m_pos.z / 50.0;
    
    // update view and projection matrices
    m_view = glm::lookAt(m_pos, m_pos + glm::dvec3(0, 0, -1), glm::dvec3(0, 1, 0));
    m_proj = glm::perspective(fovy, m_aspect, near, m_pos.z + 1.0);

    m_dirty = true;

}

const glm::dmat4 View::getViewProjectionMatrix() const {
    return m_proj * m_view;
}

glm::dmat2 View::getBoundsRect() const {

    double hw = m_width * 0.5;
    double hh = m_height * 0.5;
    return glm::dmat2(m_pos.x - hw, m_pos.y - hh, m_pos.x + hw, m_pos.y + hh);

}

const std::set<TileID>& View::getVisibleTiles() {

    if (!m_dirty) {
        return m_visibleTiles;
    }

    m_visibleTiles.clear();

    float tileSize = 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);
    float invTileSize = 1.0 / tileSize;

    float vpLeftEdge = m_pos.x - m_width * 0.5 + MapProjection::HALF_CIRCUMFERENCE;
    float vpRightEdge = vpLeftEdge + m_width;
    float vpBottomEdge = -m_pos.y - m_height * 0.5 + MapProjection::HALF_CIRCUMFERENCE;
    float vpTopEdge = vpBottomEdge + m_height;

    int tileX = (int) vpLeftEdge * invTileSize;
    int tileY = (int) vpBottomEdge * invTileSize;

    float x = tileX * tileSize;
    float y = tileY * tileSize;

    while (x < vpRightEdge) {

        while (y < vpTopEdge) {

            m_visibleTiles.insert(TileID(tileX, tileY, m_zoom));
            tileY++;
            y += tileSize;

        }

        tileY = (int) vpBottomEdge * invTileSize;
        y = tileY * tileSize;

        tileX++;
        x += tileSize;
    }

    m_dirty = false;

    return m_visibleTiles;

}
