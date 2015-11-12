#include "view.h"

#include "platform.h"
#include "tangram.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include <cmath>
#include <functional>

namespace Tangram {

constexpr float View::s_maxZoom; // Create a stack reference to the static member variable

double invLodFunc(double d) {
    return exp2(d) - 1.0;
}

View::View(int _width, int _height, ProjectionType _projType) :
    m_obliqueAxis(0, 1),
    m_width(0),
    m_height(0),
    m_type(CameraType::perspective),
    m_changed(false) {

    setMapProjection(_projType);
    setSize(_width, _height);
    setZoom(m_initZoom); // Arbitrary zoom for testing

    setPosition(0.0, 0.0);
}

void View::setMapProjection(ProjectionType _projType) {

    switch(_projType) {
        case ProjectionType::mercator:
            m_projection.reset(new MercatorProjection());
            break;
        default:
            LOGE("Not a valid map projection specified.\n Setting map projection to mercator by default");
            m_projection.reset(new MercatorProjection());
            break;
    }

    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

const MapProjection& View::getMapProjection() const {
    return *m_projection.get();
}

void View::setPixelScale(float _pixelsPerPoint) {

    m_pixelScale = _pixelsPerPoint;
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

void View::setSize(int _width, int _height) {

    m_vpWidth = _width;
    m_vpHeight = _height;
    m_aspect = (float)_width / (float)_height;
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

    // Screen space orthographic projection matrix, top left origin, y pointing down
    m_orthoViewport = glm::ortho(0.f, (float)m_vpWidth, (float)m_vpHeight, 0.f, -1.f, 1.f);

}

bool contains(const glm::dvec2& _point, const BoundingBox& _bounds) {
    if ( _point.y < _bounds.min.y || _point.y > _bounds.max.y ) { return false; }
    return true;
}

bool View::checkMapBound() {

    auto mapBounds = m_projection->MapBounds();

    // Make sure world space view trapezoid bounds are within mapBounds
    // If any of the 4 view trapezoid bounds are off the mapBounds, do not set the view Position
    glm::dvec2 bottomLeft = { 0.f, m_vpHeight};
    glm::dvec2 bottomRight = { m_vpWidth, m_vpHeight};
    glm::dvec2 topRight = { m_vpWidth, 0.f};
    glm::dvec2 topLeft = { 0.f, 0.f};

    screenToGroundPlane(bottomLeft.x, bottomLeft.y);
    bottomLeft += glm::dvec2(m_pos.x, m_pos.y);
    if (!contains(bottomLeft, mapBounds)) { return false; }

    screenToGroundPlane(bottomRight.x, bottomRight.y);
    bottomRight += glm::dvec2(m_pos.x, m_pos.y);
    if (!contains(bottomRight, mapBounds)) { return false; }

    screenToGroundPlane(topRight.x, topRight.y);
    topRight += glm::dvec2(m_pos.x, m_pos.y);
    if (!contains(topRight, mapBounds)) { return false; }

    screenToGroundPlane(topLeft.x, topLeft.y);
    topLeft += glm::dvec2(m_pos.x, m_pos.y);
    if (!contains(topLeft, mapBounds)) { return false; }

    return true;
}

void View::setPosition(double _x, double _y) {

    m_pos.x = _x;
    m_pos.y = _y;
    m_dirtyTiles = true;

}

void View::setZoom(float _z) {
    static float lastZoom = 0.f;

    // ensure zoom value is allowed
    m_zoom = glm::clamp(_z, s_minZoom, s_maxZoom);
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

    float zoomDir = copysign(1.0, lastZoom - m_zoom);
    m_next_zoom = floor(m_zoom - zoomDir);

    lastZoom = m_zoom;
}

void View::setRoll(float _roll) {

    m_roll = glm::mod(_roll, (float)TWO_PI);
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

void View::setPitch(float _pitch) {

    float max_pitch = HALF_PI;
    if (m_type != CameraType::perspective) {
        // Prevent projection plane from intersecting ground plane
        max_pitch = atan2(m_pos.z, m_height * .5f);
    }
    m_pitch = glm::clamp(_pitch, 0.f, max_pitch);
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

void View::translate(double _dx, double _dy) {

    setPosition(m_pos.x + _dx, m_pos.y + _dy);

}

void View::zoom(float _dz) {

    setZoom(m_zoom + _dz);

}

void View::roll(float _droll) {

    setRoll(m_roll + _droll);

}

void View::pitch(float _dpitch) {

    setPitch(m_pitch + _dpitch);

}

void View::update() {

    if (m_dirtyMatrices) {

        updateMatrices(); // Resets dirty flag
        setPitch(m_pitch); // Ensure pitch is still valid for viewport
        m_changed = true;

    }

    bool inBounds = checkMapBound();

    if(!inBounds) {

        // Reset view values to previous legal values
        m_pos = m_pos_prev;
        m_roll = m_roll_prev;
        m_zoom = m_zoom_prev;
        m_pitch = m_pitch_prev;
        m_dirtyMatrices = true;
        m_dirtyTiles = true;
        updateMatrices();

        inBounds = checkMapBound();
    }

    m_pos_prev = m_pos;
    m_roll_prev = m_roll;
    m_zoom_prev = m_zoom;
    m_pitch_prev = m_pitch;

    if (m_dirtyTiles && !Tangram::getDebugFlag(Tangram::DebugFlags::freeze_tiles)) {

        updateTiles(); // Resets dirty flag
        m_changed = true;

    }

    // If the viewport dimention change and the view trapezoid is not within the mapbound,
    // zoom in to get view trapezoid within map bounds
    if(!inBounds) {
        m_zoom_prev += 1;
        requestRender();
    }
}

glm::dmat2 View::getBoundsRect() const {

    double hw = m_width * 0.5;
    double hh = m_height * 0.5;
    return glm::dmat2(m_pos.x - hw, m_pos.y - hh, m_pos.x + hw, m_pos.y + hh);

}

double View::screenToGroundPlane(float& _screenX, float& _screenY) {
    double x = _screenX, y = _screenY;
    double t = screenToGroundPlane(x, y);
    _screenX = x;
    _screenY = y;
    return t;
}

double View::screenToGroundPlane(double& _screenX, double& _screenY) {

    if (m_dirtyMatrices) { updateMatrices(); } // Need the view matrices to be up-to-date

    // Cast a ray and find its intersection with the z = 0 plane,
    // following the technique described here: http://antongerdelan.net/opengl/raycasting.html

    glm::dvec4 target_clip = { 2. * _screenX / m_vpWidth - 1., 1. - 2. * _screenY / m_vpHeight, -1., 1. };
    glm::dvec4 target_world = m_invViewProj * target_clip;
    target_world /= target_world.w;

    glm::dvec4 origin_world;
    switch (m_type) {
        case CameraType::perspective:
            origin_world = glm::dvec4(m_eye, 1);
            break;
        case CameraType::isometric:
        case CameraType::flat:
            origin_world = m_invViewProj * (target_clip * glm::dvec4(1, 1, 0, 1));
            break;
    }

    glm::dvec4 ray_world = target_world - origin_world;

    double t = 0; // Distance along ray to ground plane
    if (ray_world.z != 0.f) {
        t = -origin_world.z / ray_world.z;
    }

    ray_world *= std::abs(t);

    // Determine the maximum distance from the view position at which tiles can be drawn; If the projected point
    // is farther than this maximum or if the point is above the horizon (t < 0) then we set the distance of the
    // point to always be this maximum distance.
    double maxTileDistance = invLodFunc(MAX_LOD + 1) * 2.0 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);
    double rayDistanceXY = sqrt(ray_world.x * ray_world.x + ray_world.y * ray_world.y);
    if (rayDistanceXY > maxTileDistance || t < 0) {
        ray_world *= maxTileDistance / rayDistanceXY;
    }

    _screenX = ray_world.x + origin_world.x;
    _screenY = ray_world.y + origin_world.y;

    return t;
}

float View::pixelsPerMeter() const {

    double metersPerTile = MapProjection::HALF_CIRCUMFERENCE * exp2(-m_zoom);
    return m_pixelsPerTile / metersPerTile;
}

void View::updateMatrices() {

    // find dimensions of tiles in world space at new zoom level
    float worldTileSize = 2 * MapProjection::HALF_CIRCUMFERENCE * pow(2, -m_zoom);

    // viewport height in world space is such that each tile is [m_pixelsPerTile] px square in screen space
    float screenTileSize = m_pixelsPerTile * m_pixelScale;
    m_height = (float)m_vpHeight * worldTileSize / screenTileSize;
    m_width = m_height * m_aspect;

    // set vertical field-of-view
    float fovy = PI * 0.25;

    // we assume portrait orientation by default, so in landscape
    // mode we scale the vertical FOV such that the wider dimension
    // gets the intended FOV
    if (m_width > m_height) {
        fovy /= m_aspect;
    }

    // set camera z to produce desired viewable area
    m_pos.z = m_height * 0.5 / tan(fovy * 0.5);

    m_eye = glm::rotateZ(glm::rotateX(glm::vec3(0.f, 0.f, m_pos.z), m_pitch), m_roll);
    glm::vec3 at = { 0.f, 0.f, 0.f };
    glm::vec3 up = glm::rotateZ(glm::rotateX(glm::vec3(0.f, 1.f, 0.f), m_pitch), m_roll);

    // Generate view matrix
    m_view = glm::lookAt(m_eye, at, up);

    float maxTileDistance = worldTileSize * invLodFunc(MAX_LOD + 1);
    float near = m_pos.z / 50.f;
    float far = 1;
    float hw = 0.5 * m_width;
    float hh = 0.5 * m_height;

    // Generate projection matrix based on camera type
    switch (m_type) {
        case CameraType::perspective:
            far = 2. * m_pos.z / std::max(0., cos(m_pitch + 0.5 * fovy));
            far = std::min(far, maxTileDistance);
            m_proj = glm::perspective(fovy, m_aspect, near, far);
            break;
        case CameraType::isometric:
        case CameraType::flat:
            far = 2. * (m_pos.z + hh * std::abs(tan(m_pitch)));
            far = std::min(far, maxTileDistance);
            m_proj = glm::ortho(-hw, hw, -hh, hh, near, far);
            break;
    }

    if (m_type == CameraType::isometric) {
        // Add the oblique projection scaling factors to the view matrix
        m_view[2][0] += m_obliqueAxis.x;
        m_view[2][1] += m_obliqueAxis.y;
    }

    m_viewProj = m_proj * m_view;
    m_invViewProj = glm::inverse(m_viewProj);

    // The matrix that transforms normals from world space to camera space is the transpose of the inverse of the view matrix,
    // but since our view matrix is orthonormal transposing is equivalent to inverting, so the normal matrix is just the
    // original view matrix (cropped to the top-left 3 rows and columns, since we're applying it to 3d vectors)
    m_normalMatrix = glm::mat3(m_view);

    m_dirtyMatrices = false;

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
        scanLine(floor(x1), ceil(x0), y, _s);
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

    int zoom = int(m_zoom);
    int maxTileIndex = 1 << zoom;

    // Bounds of view trapezoid in world space (i.e. view frustum projected onto z = 0 plane)
    glm::dvec2 viewBL = { 0.f,       m_vpHeight }; // bottom left
    glm::dvec2 viewBR = { m_vpWidth, m_vpHeight }; // bottom right
    glm::dvec2 viewTR = { m_vpWidth, 0.f        }; // top right
    glm::dvec2 viewTL = { 0.f,       0.f        }; // top left

    double t0 = screenToGroundPlane(viewBL.x, viewBL.y);
    double t1 = screenToGroundPlane(viewBR.x, viewBR.y);
    double t2 = screenToGroundPlane(viewTR.x, viewTR.y);
    double t3 = screenToGroundPlane(viewTL.x, viewTL.y);

    // if all of our raycasts have a negative intersection distance, we have no area to cover
    if (t0 < .0 && t1 < 0. && t2 < 0. && t3 < 0.) {
        return;
    }

    // Transformation from world space to tile space
    double hc = MapProjection::HALF_CIRCUMFERENCE;
    double invTileSize = double(maxTileIndex) / (hc * 2);
    glm::dvec2 tileSpaceOrigin(-hc, hc);
    glm::dvec2 tileSpaceAxes(invTileSize, -invTileSize);

    // Bounds of view trapezoid in tile space
    glm::dvec2 a = (glm::dvec2(viewBL.x + m_pos.x, viewBL.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 b = (glm::dvec2(viewBR.x + m_pos.x, viewBR.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 c = (glm::dvec2(viewTR.x + m_pos.x, viewTR.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 d = (glm::dvec2(viewTL.x + m_pos.x, viewTL.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;

    // Location of the view center in tile space
    glm::dvec2 e = (glm::dvec2(m_pos.x + m_eye.x, m_pos.y + m_eye.y) - tileSpaceOrigin) * tileSpaceAxes;

    int imax = std::numeric_limits<int>::max();
    int imin = std::numeric_limits<int>::min();

    // Distance thresholds in tile space for levels of detail:
    // Element [n] in each array is the minimum tile index at which level-of-detail n
    // should be applied in that direction.
    int x_limit_pos[MAX_LOD] = { imax };
    int x_limit_neg[MAX_LOD] = { imin };
    int y_limit_pos[MAX_LOD] = { imax };
    int y_limit_neg[MAX_LOD] = { imin };

    if (m_type == CameraType::perspective) {

        // Determine zoom reduction for tiles far from the center of view
        double tilesAtFullZoom = std::max(m_width, m_height) * invTileSize * 0.5;
        double viewCenterX = (m_pos.x + hc) * invTileSize;
        double viewCenterY = (m_pos.y - hc) * -invTileSize;

        for (int i = 0; i < MAX_LOD; i++) {
            int j = i + 1;
            double r = invLodFunc(i) + tilesAtFullZoom;
            x_limit_neg[i] = ((int(viewCenterX - r) >> j) - 1) << j;
            y_limit_pos[i] = ((int(viewCenterY + r) >> j) + 1) << j;
            y_limit_neg[i] = ((int(viewCenterY - r) >> j) - 1) << j;
            x_limit_pos[i] = ((int(viewCenterX + r) >> j) + 1) << j;
        }
    }

    Scan s = [&](int x, int y) {

        int lod = 0;
        while (lod < MAX_LOD && x >= x_limit_pos[lod]) { lod++; }
        while (lod < MAX_LOD && x <  x_limit_neg[lod]) { lod++; }
        while (lod < MAX_LOD && y >= y_limit_pos[lod]) { lod++; }
        while (lod < MAX_LOD && y <  y_limit_neg[lod]) { lod++; }

        x >>= lod;
        y >>= lod;
        int z = glm::clamp((zoom - lod), 0, (int)s_maxZoom);

        // Wrap x to the range [0, (1 << z))
        int wx = x & ((1 << z) - 1);
        int wrap = (x - wx) >> zoom;

        m_visibleTiles.emplace(wx, y, z, wrap);

    };

    // Rasterize view trapezoid into tiles
    scanTriangle(a, b, c, 0, maxTileIndex, s);
    scanTriangle(c, d, a, 0, maxTileIndex, s);

    // Rasterize the area bounded by the point under the view center and the two nearest corners
    // of the view trapezoid. This is necessary to not cull any geometry with height in these tiles
    // (which should remain visible, even though the base of the tile is not).
    scanTriangle(a, b, e, 0, maxTileIndex, s);

    m_dirtyTiles = false;

}

}
