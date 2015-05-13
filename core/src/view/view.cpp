#include "view.h"

#include <cmath>
#include <functional>

#include "util/tileID.h"
#include "platform.h"
#include "tangram.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

#define MAX_LOD 6

constexpr float View::s_maxZoom; // Create a stack reference to the static member variable

double invLodFunc(double d) {
    return exp2(d) - 1.0;
}

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
    m_pitch = glm::clamp(_pitch, 0.f, (float)M_PI);
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
    
    float t = 0; // Distance along ray to ground plane
    if (ray_world.z != 0.f) {
        t = -m_pos.z / ray_world.z;
    }

    ray_world *= fabs(t);
    
    // Determine the maximum distance from the view position at which tiles can be drawn; If the projected point 
    // is farther than this maximum or if the point is above the horizon (t < 0) then we set the distance of the
    // point to always be this maximum distance. 
    float maxTileDistance = invLodFunc(MAX_LOD + 1) * 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);
    float rayDistanceXY = sqrtf(ray_world.x * ray_world.x + ray_world.y * ray_world.y);
    if (rayDistanceXY > maxTileDistance || t < 0) {
        ray_world *= maxTileDistance / rayDistanceXY;
    }
    
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
    
    // set near clipping distance as a function of camera z
    // TODO: this is a simple heuristic that deserves more thought
    float near = m_pos.z / 50.0;
    
    // set far clipping distance to the distance of the intersection of
    // the "top" face of the view frustum with the ground plane
    
    // NOTE: far here can go to infinity and hence a std::min below!
    float far = m_pos.z / cos(m_pitch + 0.5 * fovy);
    
    // limit the far clipping distance to be no greater than the maximum
    // distance of visible tiles
    float maxTileDistance = worldTileSize * invLodFunc(MAX_LOD + 1);
    far = std::min(far, maxTileDistance);
    
    glm::vec3 eye = { 0.f, 0.f, 0.f };
    glm::vec3 at = glm::rotateZ(glm::rotateX(glm::vec3(0.f, 0.f, -1.f), m_pitch), m_roll);
    glm::vec3 up = glm::rotateZ(glm::rotateX(glm::vec3(0.f, 1.f, 0.f), m_pitch), m_roll);
    
    // update view and projection matrices
    m_view = glm::lookAt(eye, at, up);
    m_proj = glm::perspective(fovy, m_aspect, near, far);
    m_viewProj = m_proj * m_view;
    m_invViewProj = glm::inverse(m_viewProj);
    
    // The matrix that transforms normals from world space to camera space is the transpose of the inverse of the view matrix,
    // but since our view matrix is orthonormal transposing is equivalent to inverting, so the normal matrix is just the
    // original view matrix (cropped to the top-left 3 rows and columns, since we're applying it to 3d vectors)
    m_normalMatrix = glm::mat3(m_view);
    
}

// Triangle rasterization adapted from Polymaps: https://github.com/simplegeo/polymaps/blob/master/src/Layer.js#L333-L383

struct edge { // An edge between two points; oriented such that y is non-decreasing
    double x0 = 0, y0 = 0;
    double x1 = 0, y1 = 0;
    double dx = 0, dy = 0;
    
    edge(glm::dvec2 _a, glm::dvec2 _b) {
        if (_a.y > _b.y) { std::swap(_a, _b); }
        x0 = _a.x;
        y0 = _a.y;
        x1 = _b.x;
        y1 = _b.y;
        dx = x1 - x0;
        dy = y1 - y0;
    }
};

typedef std::function<void (int, int)> Scan;

static void scanLine(int _x0, int _x1, int _y, Scan _s) {
    
    for (int x = _x0; x < _x1; x++) {
        _s(x, _y);
    }
}

static void scanSpan(edge _e0, edge _e1, int _min, int _max, Scan& _s) {
    
    // _e1 has a shorter y-span, so we'll use it to limit our y coverage
    int y0 = fmax(_min, floor(_e1.y0));
    int y1 = fmin(_max, ceil(_e1.y1));
    
    // sort edges by x-coordinate
    if (_e0.x0 == _e1.x0 && _e0.y0 == _e1.y0) {
        if (_e0.x0 + _e1.dy / _e0.dy * _e0.dx < _e1.x1) { std::swap(_e0, _e1); }
    } else {
        if (_e0.x1 - _e1.dy / _e0.dy * _e0.dx < _e1.x0) { std::swap(_e0, _e1); }
    }
    
    // scan lines!
    double m0 = _e0.dx / _e0.dy;
    double m1 = _e1.dx / _e1.dy;
    double d0 = _e0.dx > 0 ? 1.0 : 0.0;
    double d1 = _e1.dx < 0 ? 1.0 : 0.0;
    for (int y = y0; y < y1; y++) {
        double x0 = m0 * fmax(0.0, fmin(_e0.dy, y + d0 - _e0.y0)) + _e0.x0;
        double x1 = m1 * fmax(0.0, fmin(_e1.dy, y + d1 - _e1.y0)) + _e1.x0;
        scanLine(fmax(_min, floor(x1)), fmin(_max, ceil(x0)), y, _s);
    }
    
}

static void scanTriangle(glm::dvec2& _a, glm::dvec2& _b, glm::dvec2& _c, int _min, int _max, Scan& _s) {
    
    edge ab = edge(_a, _b);
    edge bc = edge(_b, _c);
    edge ca = edge(_c, _a);
    
    // place edge with greatest y distance in ca
    if (ab.dy > ca.dy) { std::swap(ab, ca); }
    if (bc.dy > ca.dy) { std::swap(bc, ca); }
    
    // scan span! scan span!
    if (ab.dy > 0) { scanSpan(ca, ab, _min, _max, _s); }
    if (bc.dy > 0) { scanSpan(ca, bc, _min, _max, _s); }
    
}

void View::updateTiles() {
    
    m_visibleTiles.clear();
    
    // Bounds of view trapezoid in world space (i.e. view frustum projected onto z = 0 plane)
    glm::vec2 viewBL = { 0.f,       m_vpHeight }; // bottom left
    glm::vec2 viewBR = { m_vpWidth, m_vpHeight }; // bottom right
    glm::vec2 viewTR = { m_vpWidth, 0.f        }; // top right
    glm::vec2 viewTL = { 0.f,       0.f        }; // top left
    
    screenToGroundPlane(viewBL.x, viewBL.y);
    screenToGroundPlane(viewBR.x, viewBR.y);
    screenToGroundPlane(viewTR.x, viewTR.y);
    screenToGroundPlane(viewTL.x, viewTL.y);
    
    // Transformation from world space to tile space
    double hc = MapProjection::HALF_CIRCUMFERENCE;
    double invTileSize = double(1 << int(m_zoom)) / (hc * 2);
    glm::dvec2 tileSpaceOrigin(-hc, hc);
    glm::dvec2 tileSpaceAxes(invTileSize, -invTileSize);
    
    // Bounds of view trapezoid in tile space
    glm::dvec2 a = (glm::dvec2(viewBL.x + m_pos.x, viewBL.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 b = (glm::dvec2(viewBR.x + m_pos.x, viewBR.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 c = (glm::dvec2(viewTR.x + m_pos.x, viewTR.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 d = (glm::dvec2(viewTL.x + m_pos.x, viewTL.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;

    // Location of the view center in tile space
    glm::dvec2 e = (glm::dvec2(m_pos.x, m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    
    // Determine zoom reduction for tiles far from the center of view
    double tilesAtFullZoom = std::max(m_width, m_height) * invTileSize * 0.5;
    double viewCenterX = (m_pos.x + hc) * invTileSize;
    double viewCenterY = (m_pos.y - hc) * -invTileSize;
    
    int x_l_pos[MAX_LOD] = { 0 };
    int x_l_neg[MAX_LOD] = { 0 };
    int y_l_pos[MAX_LOD] = { 0 };
    int y_l_neg[MAX_LOD] = { 0 };
    
    for (int i = 0; i < MAX_LOD; i++) {
        int j = i + 1;
        x_l_neg[i] = ((int(viewCenterX - tilesAtFullZoom - invLodFunc(i)) >> j) - 1) << j;
        y_l_pos[i] = ((int(viewCenterY + tilesAtFullZoom + invLodFunc(i)) >> j) + 1) << j;
        y_l_neg[i] = ((int(viewCenterY - tilesAtFullZoom - invLodFunc(i)) >> j) - 1) << j;
        x_l_pos[i] = ((int(viewCenterX + tilesAtFullZoom + invLodFunc(i)) >> j) + 1) << j;
    }
    
    Scan s = [&](int x, int y) {

        int lod = 0;
        while (lod < MAX_LOD && x >= x_l_pos[lod]) { lod++; }
        while (lod < MAX_LOD && x <  x_l_neg[lod]) { lod++; }
        while (lod < MAX_LOD && y >= y_l_pos[lod]) { lod++; }
        while (lod < MAX_LOD && y <  y_l_neg[lod]) { lod++; }
        
        int z = int(m_zoom);
        
        x >>= lod;
        y >>= lod;
        z = glm::clamp((z-lod), 0, (int)s_maxZoom);
        
        m_visibleTiles.emplace(x, y, z);
        
    };
    
    // Rasterize view trapezoid into tiles
    int maxTileIndex = 1 << int(m_zoom);
    scanTriangle(a, b, c, 0, maxTileIndex, s);
    scanTriangle(c, d, a, 0, maxTileIndex, s);

    // Rasterize the area bounded by the point under the view center and the two nearest corners 
    // of the view trapezoid. This is necessary to not cull any geometry with height in these tiles
    // (which should remain visible, even though the base of the tile is not).
    scanTriangle(a, b, e, 0, maxTileIndex, s);

}
