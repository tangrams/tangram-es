#include "view/view.h"

#include "log.h"
#include "scene/stops.h"
#include "util/rasterize.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include <cmath>

#define MAX_LOD 6

namespace Tangram {

double invLodFunc(double d) {
    return exp2(d) - 1.0;
}

View::View(int _width, int _height) :
    m_obliqueAxis(0, 1),
    m_width(0),
    m_height(0),
    m_type(CameraType::perspective),
    m_dirtyMatrices(true),
    m_dirtyTiles(true),
    m_changed(false) {

    auto bounds = MapProjection::mapProjectedMetersBounds();
    m_constraint.setLimitsY(bounds.min.y, bounds.max.y);

    setSize(_width, _height);
    setZoom(m_zoom);
    setPosition(0.0, 0.0);
}

void View::setPixelScale(float _pixelsPerPoint) {

    m_pixelScale = _pixelsPerPoint;
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

void View::setCamera(const Camera& _camera) {
    setCameraType(_camera.type);

    switch (_camera.type) {
    case CameraType::perspective:
        setVanishingPoint(_camera.vanishingPoint.x, _camera.vanishingPoint.y);
        if (_camera.fovStops) {
            setFieldOfViewStops(_camera.fovStops);
        } else {
            setFieldOfView(_camera.fieldOfView);
        }
        break;
    case CameraType::isometric:
        setObliqueAxis(_camera.obliqueAxis.x, _camera.obliqueAxis.y);
        break;
    case CameraType::flat:
        break;
    }

    if (_camera.maxTiltStops) {
        setMaxPitchStops(_camera.maxTiltStops);
    } else {
        setMaxPitch(_camera.maxTilt);
    }
}

void View::setCameraType(CameraType _type) {

    m_type = _type;
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

ViewState View::state() const {

    return {
        m_changed,
        glm::dvec2(m_pos.x, m_pos.y),
        m_zoom,
        powf(2.f, m_zoom),
        m_zoom - std::floor(m_zoom),
        glm::vec2(m_vpWidth, m_vpHeight),
        (float)MapProjection::tileSize() * m_pixelScale
    };
}

void View::setSize(int _width, int _height) {

    m_vpWidth = std::max(_width, 1);
    m_vpHeight = std::max(_height, 1);
    m_aspect = (float)m_vpWidth/ (float)m_vpHeight;
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

    // Screen space orthographic projection matrix, top left origin, y pointing down
    m_orthoViewport = glm::ortho(0.f, (float)m_vpWidth, (float)m_vpHeight, 0.f, -1.f, 1.f);

}

void View::setFieldOfView(float radians) {

    m_fov = radians;
    m_fovStops = nullptr;
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

void View::setFieldOfViewStops(std::shared_ptr<Stops> stops) {

    m_fovStops = stops;
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

float View::getFieldOfView() const {

    if (m_fovStops) {
        return m_fovStops->evalFloat(m_zoom);
    }
    return m_fov;

}

void View::setFocalLength(float length) {

    setFieldOfView(focalLengthToFieldOfView(length));

}

void View::setFocalLengthStops(std::shared_ptr<Stops> stops) {

    for (auto& frame : stops->frames) {
        float length = frame.value.get<float>();
        frame.value = focalLengthToFieldOfView(length);
    }
    setFieldOfViewStops(stops);

}

float View::getFocalLength() const {

    return fieldOfViewToFocalLength(getFieldOfView());

}

void View::setMinZoom(float minZoom) {

    m_minZoom = std::max(minZoom, 0.f);
    m_maxZoom = std::max(minZoom, m_maxZoom);
    // Set the current zoom again to validate it.
    setZoom(m_zoom);
}

void View::setMaxZoom(float maxZoom) {

    m_maxZoom = std::min(maxZoom, 20.5f);
    m_minZoom = std::min(maxZoom, m_minZoom);
    // Set the current zoom again to validate it.
    setZoom(m_zoom);
}

void View::setMaxPitch(float degrees) {

    m_maxPitch = degrees;
    m_maxPitchStops = nullptr;
    setPitch(m_pitch);

}

void View::setMaxPitchStops(std::shared_ptr<Stops> stops) {

    m_maxPitchStops = stops;
    setPitch(m_pitch);

}

float View::getMaxPitch() const {

    if (m_maxPitchStops) {
        return m_maxPitchStops->evalFloat(m_zoom);
    }
    return m_maxPitch;

}

void View::setPosition(double _x, double _y) {
    // Wrap horizontal position around the 180th meridian, which corresponds to +/- HALF_CIRCUMFERENCE meters.
    m_pos.x = _x - std::round(_x / MapProjection::EARTH_CIRCUMFERENCE_METERS) * MapProjection::EARTH_CIRCUMFERENCE_METERS;
    // Clamp vertical position to the span of the map, which is +/- HALF_CIRCUMFERENCE meters.
    m_pos.y = glm::clamp(_y, -MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS, MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS);
    m_dirtyTiles = true;
}

void View::setCenterCoordinates(Tangram::LngLat center) {
    auto meters = MapProjection::lngLatToProjectedMeters({center.longitude, center.latitude});
    setPosition(meters.x, meters.y);
}

void View::setZoom(float _z) {
    // ensure zoom value is allowed
    m_zoom = glm::clamp(_z, m_minZoom, m_maxZoom);
    m_dirtyMatrices = true;
    m_dirtyTiles = true;
}

void View::setRoll(float _roll) {

    m_roll = glm::mod(_roll, (float)TWO_PI);
    m_dirtyMatrices = true;
    m_dirtyTiles = true;

}

void View::setPitch(float _pitch) {

    m_pitch = _pitch;
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

LngLat View::getCenterCoordinates() const {
    auto center = MapProjection::projectedMetersToLngLat({m_pos.x, m_pos.y});
    return center;
}

void View::update(bool _constrainToWorldBounds) {

    m_changed = false;

    if (_constrainToWorldBounds) {
        // Approximate the view diameter in pixels by taking the maximum dimension.
        double viewDiameterPixels = std::fmax(getWidth(), getHeight()) / pixelScale();
        // Approximate the minimum zoom that keeps with view span within the drawable projection area. [1]
        double minZoom = std::log(viewDiameterPixels / MapProjection::tileSize() + 2) / std::log(2);
        if (m_zoom < minZoom) {
            m_zoom = static_cast<float>(minZoom);
        }
        // Constrain by moving map center to keep view in bounds.
        m_constraint.setRadius(0.5 * viewDiameterPixels / pixelsPerMeter());
        m_pos.x = m_constraint.getConstrainedX(m_pos.x);
        m_pos.y = m_constraint.getConstrainedY(m_pos.y);
    }

    // Ensure valid pitch angle.
    {
        float maxPitchRadians = glm::radians(getMaxPitch());
        if (m_type != CameraType::perspective) {
            // Prevent projection plane from intersecting ground plane.
            float intersectingPitchRadians = atan2(m_pos.z, m_height * .5f);
            maxPitchRadians = glm::min(maxPitchRadians, intersectingPitchRadians);
        }
        m_pitch = glm::clamp(m_pitch, 0.f, maxPitchRadians);
    }

    if (m_dirtyMatrices) {

        updateMatrices(); // Resets dirty flag
        m_changed = true;

    }

    if (m_dirtyTiles) {

        m_changed = true;
        m_dirtyTiles = false;
    }
}

glm::dmat2 View::getBoundsRect() const {

    double hw = m_width * 0.5;
    double hh = m_height * 0.5;
    return glm::dmat2(m_pos.x - hw, m_pos.y - hh, m_pos.x + hw, m_pos.y + hh);

}

double View::screenToGroundPlane(float& _screenX, float& _screenY) {

    if (m_dirtyMatrices) { updateMatrices(); } // Need the view matrices to be up-to-date

    double x = _screenX, y = _screenY;
    double t = screenToGroundPlaneInternal(x, y);
    _screenX = x;
    _screenY = y;
    return t;
}

double View::screenToGroundPlane(double& _screenX, double& _screenY) {

    if (m_dirtyMatrices) { updateMatrices(); } // Need the view matrices to be up-to-date

    return screenToGroundPlaneInternal(_screenX, _screenY);
}

glm::vec2 View::normalizedWindowCoordinates(float _x, float _y) const {
    return { _x / m_vpWidth, 1.0 - _y / m_vpHeight };
}

double View::screenToGroundPlaneInternal(double& _screenX, double& _screenY) const {

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
    double maxTileDistance = invLodFunc(MAX_LOD + 1) * 2.0 * MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS * pow(2, -m_zoom);
    double rayDistanceXY = sqrt(ray_world.x * ray_world.x + ray_world.y * ray_world.y);
    if (rayDistanceXY > maxTileDistance || t < 0) {
        ray_world *= maxTileDistance / rayDistanceXY;
    }

    _screenX = ray_world.x + origin_world.x;
    _screenY = ray_world.y + origin_world.y;

    return t;
}

float View::pixelsPerMeter() const {

    double metersPerTile = MapProjection::EARTH_CIRCUMFERENCE_METERS * exp2(-m_zoom);
    return MapProjection::tileSize() / metersPerTile;
}

float View::focalLengthToFieldOfView(float length) {
    return 2.f * atanf(1.f / length);
}

float View::fieldOfViewToFocalLength(float radians) {
    return 1.f / tanf(radians / 2.f);
}

void View::updateMatrices() {

    // find dimensions of tiles in world space at new zoom level
    float worldTileSize = MapProjection::EARTH_CIRCUMFERENCE_METERS * exp2(-m_zoom);

    // viewport height in world space is such that each tile is [m_pixelsPerTile] px square in screen space
    float screenTileSize = MapProjection::tileSize() * m_pixelScale;
    m_height = (float)m_vpHeight * worldTileSize / screenTileSize;
    m_width = m_height * m_aspect;

    // set vertical field-of-view
    float fovy = getFieldOfView();

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
            // Adjust for vanishing point.
            m_proj[2][0] -= m_vanishingPoint.x / getWidth();
            m_proj[2][1] -= m_vanishingPoint.y / getHeight();
            break;
        case CameraType::isometric:
        case CameraType::flat:
            far = 2. * (m_pos.z + hh * std::abs(tan(m_pitch)));
            far = std::min(far, maxTileDistance);
            m_proj = glm::ortho(-hw, hw, -hh, hh, near, far);
            break;
    }

    if (m_type == CameraType::isometric) {
        glm::mat4 shear = m_view;

        // Add the oblique projection scaling factors to the shear matrix
        shear[2][0] += m_obliqueAxis.x;
        shear[2][1] += m_obliqueAxis.y;

        // Remove the view from the shear matrix so we don't apply it two times
        shear *= glm::inverse(m_view);

        // Inject the shear in the projection matrix
        m_proj *= shear;
    }

    m_viewProj = m_proj * m_view;
    m_invViewProj = glm::inverse(m_viewProj);

    // The matrix that transforms normals from world space to camera space is the transpose of the inverse of the view matrix,
    // but since our view matrix is orthonormal transposing is equivalent to inverting, so the normal matrix is just the
    // original view matrix (cropped to the top-left 3 rows and columns, since we're applying it to 3d vectors)
    m_normalMatrix = glm::mat3(m_view);
    m_invNormalMatrix = glm::inverse(m_normalMatrix);

    m_dirtyMatrices = false;

}

glm::vec2 View::lngLatToScreenPosition(double lng, double lat, bool& clipped) {

    if (m_dirtyMatrices) { updateMatrices(); } // Need the view matrices to be up-to-date

    glm::dvec2 absoluteMeters = MapProjection::lngLatToProjectedMeters({lng, lat});
    glm::dvec2 relativeMeters = getRelativeMeters(absoluteMeters);
    glm::dvec4 worldPosition(relativeMeters, 0.0, 1.0);

    glm::vec2 screenPosition = worldToScreenSpace(m_viewProj, worldPosition, {m_vpWidth, m_vpHeight}, clipped);

    return screenPosition;
}

LngLat View::screenPositionToLngLat(float x, float y, bool& _intersection) {

    if (m_dirtyMatrices) { updateMatrices(); } // Need the view matrices to be up-to-date

    double dx = x, dy = y;
    double distance = screenToGroundPlaneInternal(dx, dy);
    _intersection = (distance >= 0);
    glm::dvec2 meters(dx + m_pos.x, dy + m_pos.y);
    LngLat lngLat = MapProjection::projectedMetersToLngLat(meters);
    return lngLat.wrapped();
}

glm::dvec2 View::getRelativeMeters(glm::dvec2 projectedMeters) const {
    double dx = projectedMeters.x - m_pos.x;
    double dy = projectedMeters.y - m_pos.y;
    // If the position is closer when wrapped around the 180th meridian, then wrap it.
    if (dx > MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS) {
        dx -= MapProjection::EARTH_CIRCUMFERENCE_METERS;
    } else if (dx < -MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS) {
        dx += MapProjection::EARTH_CIRCUMFERENCE_METERS;
    }
    return {dx, dy};
}

void View::getVisibleTiles(const std::function<void(TileID)>& _tileCb) const {

    int zoom = getIntegerZoom();
    int maxTileIndex = 1 << zoom;

    // Bounds of view trapezoid in world space (i.e. view frustum projected onto z = 0 plane)
    glm::dvec2 viewBL = { 0.f,       m_vpHeight }; // bottom left
    glm::dvec2 viewBR = { m_vpWidth, m_vpHeight }; // bottom right
    glm::dvec2 viewTR = { m_vpWidth, 0.f        }; // top right
    glm::dvec2 viewTL = { 0.f,       0.f        }; // top left

    double t0 = screenToGroundPlaneInternal(viewBL.x, viewBL.y);
    double t1 = screenToGroundPlaneInternal(viewBR.x, viewBR.y);
    double t2 = screenToGroundPlaneInternal(viewTR.x, viewTR.y);
    double t3 = screenToGroundPlaneInternal(viewTL.x, viewTL.y);

    // if all of our raycasts have a negative intersection distance, we have no area to cover
    if (t0 < .0 && t1 < 0. && t2 < 0. && t3 < 0.) {
        return;
    }

    // Transformation from world space to tile space
    double hc = MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS;
    double invTileSize = double(maxTileIndex) / MapProjection::EARTH_CIRCUMFERENCE_METERS;
    glm::dvec2 tileSpaceOrigin(-hc, hc);
    glm::dvec2 tileSpaceAxes(invTileSize, -invTileSize);

    // Bounds of view trapezoid in tile space
    glm::dvec2 a = (glm::dvec2(viewBL.x + m_pos.x, viewBL.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 b = (glm::dvec2(viewBR.x + m_pos.x, viewBR.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 c = (glm::dvec2(viewTR.x + m_pos.x, viewTR.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;
    glm::dvec2 d = (glm::dvec2(viewTL.x + m_pos.x, viewTL.y + m_pos.y) - tileSpaceOrigin) * tileSpaceAxes;

    // Location of the view center in tile space
    glm::dvec2 e = (glm::dvec2(m_pos.x + m_eye.x, m_pos.y + m_eye.y) - tileSpaceOrigin) * tileSpaceAxes;

    static const int imax = std::numeric_limits<int>::max();
    static const int imin = std::numeric_limits<int>::min();

    // Scan options - avoid heap allocation for std::function
    // [1] http://www.drdobbs.com/cpp/efficient-use-of-lambda-expressions-and/232500059?pgno=2
    struct ScanParams {
        explicit ScanParams(int _zoom, int _maxZoom) : zoom(_zoom), maxZoom(_maxZoom) {}

        int zoom;
        int maxZoom;

        // Distance thresholds in tile space for levels of detail:
        // Element [n] in each array is the minimum tile index at which level-of-detail n
        // should be applied in that direction.
        int x_limit_pos[MAX_LOD] = { imax };
        int x_limit_neg[MAX_LOD] = { imin };
        int y_limit_pos[MAX_LOD] = { imax };
        int y_limit_neg[MAX_LOD] = { imin };

        glm::ivec4 last = glm::ivec4{-1};
    };

    ScanParams opt{ zoom, static_cast<int>(m_maxZoom) };

    if (m_type == CameraType::perspective) {

        // Determine zoom reduction for tiles far from the center of view
        double tilesAtFullZoom = std::max(m_width, m_height) * invTileSize * 0.5;
        double viewCenterX = (m_pos.x + hc) * invTileSize;
        double viewCenterY = (m_pos.y - hc) * -invTileSize;

        for (int i = 0; i < MAX_LOD; i++) {
            int j = i + 1;
            double r = invLodFunc(i) + tilesAtFullZoom;
            opt.x_limit_neg[i] = ((int(viewCenterX - r) >> j) - 1) << j;
            opt.y_limit_pos[i] = ((int(viewCenterY + r) >> j) + 1) << j;
            opt.y_limit_neg[i] = ((int(viewCenterY - r) >> j) - 1) << j;
            opt.x_limit_pos[i] = ((int(viewCenterX + r) >> j) + 1) << j;
        }
    }

    Rasterize::ScanCallback s = [&opt, &_tileCb](int x, int y) {

        int lod = 0;
        while (lod < MAX_LOD && x >= opt.x_limit_pos[lod]) { lod++; }
        while (lod < MAX_LOD && x <  opt.x_limit_neg[lod]) { lod++; }
        while (lod < MAX_LOD && y >= opt.y_limit_pos[lod]) { lod++; }
        while (lod < MAX_LOD && y <  opt.y_limit_neg[lod]) { lod++; }

        x >>= lod;
        y >>= lod;

        glm::ivec4 tile;
        tile.z = glm::clamp((opt.zoom - lod), 0, opt.maxZoom);

        // Wrap x to the range [0, (1 << z))
        tile.x = x & ((1 << tile.z) - 1);
        tile.y = y;

        if (tile != opt.last) {
            opt.last = tile;

            _tileCb(TileID(tile.x, tile.y, tile.z, tile.z));
        }
    };

    // Rasterize view trapezoid into tiles
    Rasterize::scanTriangle(a, b, c, 0, maxTileIndex, s);
    Rasterize::scanTriangle(c, d, a, 0, maxTileIndex, s);

    // Rasterize the area bounded by the point under the view center and the two nearest corners
    // of the view trapezoid. This is necessary to not cull any geometry with height in these tiles
    // (which should remain visible, even though the base of the tile is not).
    Rasterize::scanTriangle(a, b, e, 0, maxTileIndex, s);
}

}

// [1]
// The maximum visible span horizontally is the span covered by 2^z - 2 tiles. We consider one tile to be
// effectively not visible because at the 180th meridian it will be drawn on one side or the other, not half
// on both sides. Tile coverage is calculated from the floor() of our zoom value but here we operate on the
// continuous zoom value, so we remove one more tile to under-approximate the coverage. Algebraically we get:
// (span of view in meters) = (view diameter in pixels) * (earth circumference) / ((tile size in pixels) * 2^z)
// If we substitute the desired view span at the minimum zoom:
// (span of view in meters) = (earth circumference) * (2^z - 2) / 2^z
// We can solve for the minimum zoom:
// z = log2((view diameter in pixels) / (tile size in pixels) + 2)
