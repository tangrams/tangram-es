#include "view.h"
#include "util/tileID.h"
#include "platform.h"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/matrix_transform.hpp"

const int View::s_maxZoom; // Create a stack reference to the static member variable

View::View(int _width, int _height, ProjectionType _projType) {
    
    setMapProjection(_projType);
    setSize(_width, _height);
    setZoom(16); // Arbitrary zoom for testing
    setPosition(0.0, 0.0);
    
    m_changed = false;
    m_dirty = true;
    
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

void View::setPixelScale(float _pixelsPerPoint) {
    
    m_pixelScale = _pixelsPerPoint;
    m_dirty = true;
    
}

void View::setSize(int _width, int _height) {

    m_vpWidth = _width;
    m_vpHeight = _height;
    m_aspect = (float)_width / (float)_height;
    m_dirty = true;

}

void View::setPosition(double _x, double _y) {

    m_pos.x = _x;
    m_pos.y = _y;
    m_dirty = true;

}

void View::setZoom(int _z) {
    
    // ensure zoom value is allowed
    m_zoom = glm::clamp(_z, 0, s_maxZoom);
    m_dirty = true;
    
}

void View::translate(double _dx, double _dy) {

    setPosition(m_pos.x + _dx, m_pos.y + _dy);

}

void View::zoom(int _dz) {
    
    setZoom(m_zoom + _dz);
    
}

void View::update() {
    
    if (!m_dirty) {
        return;
    }
    
    updateMatrices();
    
    updateTiles();
    
    m_dirty = false;
    
    m_changed = true;
    
}

const glm::dmat4 View::getViewProjectionMatrix() {
    
    return m_proj * m_view;
    
}

glm::dmat2 View::getBoundsRect() const {

    double hw = m_width * 0.5;
    double hh = m_height * 0.5;
    return glm::dmat2(m_pos.x - hw, m_pos.y - hh, m_pos.x + hw, m_pos.y + hh);

}

float View::toWorldDistance(float _screenDistance) const {
    float metersPerTile = 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);
    return _screenDistance * metersPerTile / (m_pixelScale * m_pixelsPerTile);
}

const std::set<TileID>& View::getVisibleTiles() {

    return m_visibleTiles;

}

bool View::changedSinceLastCheck() {
    
    if (m_changed) {
        m_changed = false;
        return true;
    }
    return false;
    
}

void View::updateMatrices() {
    
    // find dimensions of tiles in world space at new zoom level
    float worldTileSize = 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);
    
    // viewport height in world space is such that each tile is [m_pixelsPerTile] px square in screen space
    float screenTileSize = m_pixelsPerTile * m_pixelScale;
    m_height = (float)m_vpHeight * worldTileSize / screenTileSize;
    m_width = m_height * m_aspect;
    
    // set vertical field-of-view
    double fovy = PI * 0.5;
    
    // we assume portrait orientation by default, so in landscape
    // mode we scale the vertical FOV such that the wider dimension
    // gets the intended FOV
    if (m_width > m_height) {
        fovy /= m_aspect;
    }
    
    // set camera z to produce desired viewable area
    m_pos.z = m_height * 0.5 / tan(fovy * 0.5);
    
    // set near clipping distance as a function of camera z
    // TODO: this is a simple heuristic that deserves more thought
    double near = m_pos.z / 50.0;
    
    // update view and projection matrices
    m_view = glm::lookAt(m_pos, m_pos + glm::dvec3(0, 0, -1), glm::dvec3(0, 1, 0));
    m_proj = glm::perspective(fovy, double(m_aspect), near, m_pos.z + 1.0);
    
}

void View::updateTiles() {
    
    m_visibleTiles.clear();
    
    float tileSize = 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);
    float invTileSize = 1.0 / tileSize;
    
    float vpLeftEdge = m_pos.x - m_width * 0.5 + MapProjection::HALF_CIRCUMFERENCE;
    float vpRightEdge = vpLeftEdge + m_width;
    float vpBottomEdge = -m_pos.y - m_height * 0.5 + MapProjection::HALF_CIRCUMFERENCE;
    float vpTopEdge = vpBottomEdge + m_height;
    
    int tileX = (int) fmax(0, vpLeftEdge * invTileSize);
    int tileY = (int) fmax(0, vpBottomEdge * invTileSize);
    
    float x = tileX * tileSize;
    float y = tileY * tileSize;
    
    int maxTileIndex = pow(2, m_zoom);
    
    while (x < vpRightEdge && tileX < maxTileIndex) {
        
        while (y < vpTopEdge && tileY < maxTileIndex) {
            
            m_visibleTiles.insert(TileID(tileX, tileY, m_zoom));

            tileY++;
            y += tileSize;
            
        }
        
        tileY = (int) vpBottomEdge * invTileSize;
        y = tileY * tileSize;
        
        tileX++;
        x += tileSize;
    }

}
