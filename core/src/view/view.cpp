#include "view.h"

#include <cmath>

#include "util/tileID.h"
#include "util/intersect.h"
#include "platform.h"
#include "tangram.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

constexpr float View::s_maxZoom; // Create a stack reference to the static member variable

View::View(int _width, int _height, ProjectionType _projType) {
    
    setMapProjection(_projType);
    setSize(_width, _height);
    setZoom(m_initZoom); // Arbitrary zoom for testing

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

const MapProjection& View::getMapProjection() const {
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

void View::setZoom(float _z) {
    
    // ensure zoom value is allowed
    m_zoom = glm::clamp(_z, 0.0f, s_maxZoom);
    m_dirty = true;
    
}

void View::setRoll(float _roll) {
    
    m_roll = glm::mod(_roll, (float)TWO_PI);
    m_dirty = true;
    
}

void View::setPitch(float _pitch) {

    // Clamp pitch angle (until LoD tile coverage is implemented)
    m_pitch = glm::clamp(_pitch, 0.f, 0.7f);
    m_dirty = true;

}

void View::translate(double _dx, double _dy) {

    setPosition(m_pos.x + _dx, m_pos.y + _dy);

}

void View::zoom(float _dz) {
    if(_dz > 0.0) {
        m_isZoomIn = true;
    }
    else {
        m_isZoomIn = false;
    }
    setZoom(m_zoom + _dz);
    
}

void View::roll(float _droll) {
    
    setRoll(m_roll + _droll);
    
}

void View::pitch(float _dpitch) {

    setPitch(m_pitch + _dpitch);

}

void View::orbit(float _x, float _y, float _radians) {
    
    glm::vec2 radial = { _x, _y };
    glm::vec2 displacement = glm::rotate(radial, _radians) - radial;
    translate(-displacement.x, -displacement.y);
    roll(_radians);
    
}

void View::update() {
    
    if (!m_dirty) {
        m_changed = false;
        return;
    }
    
    updateMatrices();
    
    if (!Tangram::getDebugFlag(Tangram::DebugFlags::FREEZE_TILES)) {
        
        updateTiles();
        
    }
    
    m_changed = true;
    
    m_dirty = false;
    
}

glm::dmat2 View::getBoundsRect() const {

    double hw = m_width * 0.5;
    double hh = m_height * 0.5;
    return glm::dmat2(m_pos.x - hw, m_pos.y - hh, m_pos.x + hw, m_pos.y + hh);

}

void View::screenToGroundPlane(float& _screenX, float& _screenY) const {
    
    // Cast a ray and find its intersection with the z = 0 plane,
    // following the technique described here: http://antongerdelan.net/opengl/raycasting.html
    
    glm::vec4 ray_clip = { 2.f * _screenX / m_vpWidth - 1.f, 1.f - 2.f * _screenY / m_vpHeight, -1.f, 1.f }; // Ray from camera in clip space
    glm::vec3 ray_world = glm::vec3(m_invViewProj * ray_clip); // Ray from camera in world space
    
    float t; // Distance along ray to ground plane
    if (ray_world.z != 0.f) {
        t = -m_pos.z / ray_world.z;
    } else {
        t = 0;
    }
    
    ray_world *= t;
    _screenX = ray_world.x;
    _screenY = ray_world.y;
}

const std::set<TileID>& View::getVisibleTiles() {

    return m_visibleTiles;

}

void View::updateMatrices() {
    
    // find dimensions of tiles in world space at new zoom level
    float worldTileSize = 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);
    
    // viewport height in world space is such that each tile is [m_pixelsPerTile] px square in screen space
    float screenTileSize = m_pixelsPerTile * m_pixelScale;
    m_height = (float)m_vpHeight * worldTileSize / screenTileSize;
    m_width = m_height * m_aspect;
    
    // set vertical field-of-view
    float fovy = PI * 0.5;
    
    // we assume portrait orientation by default, so in landscape
    // mode we scale the vertical FOV such that the wider dimension
    // gets the intended FOV
    if (m_width > m_height) {
        fovy /= m_aspect;
    }
    
    // set camera z to produce desired viewable area
    m_pos.z = m_height * 0.5 / tan(fovy * 0.5);
    
    // set near and far clipping distances as a function of camera z
    // TODO: this is a simple heuristic that deserves more thought
    float near = m_pos.z / 50.0;
    float far = 3. * m_pos.z / cos(m_pitch) + 1.f;
    
    glm::vec3 eye = { 0.f, 0.f, 0.f };
    glm::vec3 at = glm::rotateZ(glm::rotateX(glm::vec3(0.f, 0.f, -1.f), m_pitch), m_roll);
    glm::vec3 up = glm::rotateZ(glm::rotateX(glm::vec3(0.f, 1.f, 0.f), m_pitch), m_roll);
    
    // update view and projection matrices
    m_view = glm::lookAt(eye, at, up);
    m_proj = glm::perspective(fovy, m_aspect, near, far);
    m_viewProj = m_proj * m_view;
    m_invViewProj = glm::inverse(m_viewProj);
    
}

void View::updateTiles() {
    
    // The tile update process happens in two conceptual phases
    //
    // 1. Conservatively estimate the visible tile set with a bounding box:
    //
    //   To find this bounding box, we project the corners of the view frustum onto the ground (z = 0) plane.
    //   This creates a trapezoid (the 'view trapezoid') that we can easily create a bounding box around. Then
    //   to find the tiles in this bounding box we can simply 'scan' in x and y until we exhaust the area.
    //
    // 2. Check each tile in the bounding box for intersection with the view frustrum:
    //
    //   As we pass over each tile in the bounding box, we can take the bounds of the tile and check whether
    //   it intersects the 'view trapezoid'. Tiles that intersect the view are then added to our output list.
    //
    
    m_visibleTiles.clear();
    
    float tileSize = 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -(int)m_zoom);
    float invTileSize = 1.0 / tileSize;
    
    // Find bounds of view frustum in world space (i.e. project view frustum onto z = 0 plane)
    glm::vec2 viewBottomLeft = { 0.f, m_vpHeight };
    glm::vec2 viewBottomRight = { m_vpWidth, m_vpHeight };
    glm::vec2 viewTopRight = { m_vpWidth, 0.f };
    glm::vec2 viewTopLeft = { 0.f, 0.f };
    screenToGroundPlane(viewBottomLeft.x, viewBottomLeft.y);
    screenToGroundPlane(viewBottomRight.x, viewBottomRight.y);
    screenToGroundPlane(viewTopRight.x, viewTopRight.y);
    screenToGroundPlane(viewTopLeft.x, viewTopLeft.y);
    
    // Find axis-aligned bounding box of projected frustum (in coordinates relative to camera position)
    float aabbLeft = fminf(fminf(fminf(viewBottomLeft.x, viewBottomRight.x), viewTopLeft.x), viewTopRight.x);
    float aabbRight = fmaxf(fmaxf(fmaxf(viewBottomLeft.x, viewBottomRight.x), viewTopLeft.x), viewTopRight.x);
    float aabbBottom = fminf(fminf(fminf(viewBottomLeft.y, viewBottomRight.y), viewTopLeft.y), viewTopRight.y);
    float aabbTop = fmaxf(fmaxf(fmaxf(viewBottomLeft.y, viewBottomRight.y), viewTopLeft.y), viewTopRight.y);
    
    // Find bounds of viewable area in tile space
    float tileLeftEdge = m_pos.x + aabbLeft + MapProjection::HALF_CIRCUMFERENCE;
    float tileRightEdge = m_pos.x + aabbRight + MapProjection::HALF_CIRCUMFERENCE;
    float tileBottomEdge = -m_pos.y - aabbTop + MapProjection::HALF_CIRCUMFERENCE;
    float tileTopEdge = -m_pos.y - aabbBottom + MapProjection::HALF_CIRCUMFERENCE;
    
    int tileX = (int) fmax(0, tileLeftEdge * invTileSize);
    int tileY = (int) fmax(0, tileBottomEdge * invTileSize);
    
    float x = tileX * tileSize;
    float y = tileY * tileSize;
    
    int maxTileIndex = 1 << int(m_zoom);
    
    while (x < tileRightEdge && tileX < maxTileIndex) {
        
        while (y < tileTopEdge && tileY < maxTileIndex) {
            
            TileID id(tileX, tileY, m_zoom);
            
            glm::dvec4 bounds_abs = m_projection->TileBounds(id);
            glm::vec4 bounds_rel = glm::vec4(bounds_abs.x - m_pos.x, -bounds_abs.w - m_pos.y, bounds_abs.z - m_pos.x,  -bounds_abs.y - m_pos.y);
            // The result of TileBounds is in y-down coordinates, so to make it relative to the view position we flip the sign on y and w (ymin and ymax)
            // and then swap y and w (so that the min/max relationship remains). Finally, we subtract the view position in x and y.

            if (AABBIntersectsTrapezoid(bounds_rel, viewTopLeft, viewTopRight, viewBottomLeft, viewBottomRight)) {
                m_visibleTiles.insert(id);
            }

            tileY++;
            y += tileSize;
            
        }
        
        tileY = (int) fmax(0, tileBottomEdge * invTileSize);
        y = tileY * tileSize;
        
        tileX++;
        x += tileSize;
    }

}
