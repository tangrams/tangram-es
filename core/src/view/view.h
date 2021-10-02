#pragma once

#include "tile/tileID.h"
#include "util/mapProjection.h"
#include "util/types.h"
#include "view/viewConstraint.h"

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <functional>
#include <map.h>
#include <memory>

namespace Tangram {

enum class CameraType : uint8_t {
    perspective = 0,
    isometric,
    flat,
};

struct Stops;

struct Camera {
    CameraType type = CameraType::perspective;

    float maxTilt = 90.f;
    std::shared_ptr<Stops> maxTiltStops;

    // perspective
    glm::vec2 vanishingPoint = {0, 0};
    float fieldOfView = 0.25 * PI;
    std::shared_ptr<Stops> fovStops;

    // isometric
    glm::vec2 obliqueAxis = {0, 1};
};

struct ViewState {
    bool changedOnLastUpdate;
    glm::dvec2 center;
    float zoom;
    double zoomScale;
    float fractZoom;
    glm::vec2 viewportSize;
    float tileSize;
};

// View
// 1. Stores a representation of the current view into the map world
// 2. Determines which tiles are visible in the current view
// 3. Tracks changes in the view state to determine when new rendering is needed

class View {

public:

    View(int _width = 800, int _height = 600);

    void setCamera(const Camera& _camera);

    void setCameraType(CameraType _type);
    auto cameraType() const { return m_type; }

    void setObliqueAxis(float _x, float _y) { m_obliqueAxis = { _x, _y}; }
    auto obliqueAxis() const { return m_obliqueAxis; }

    void setVanishingPoint(float x, float y) { m_vanishingPoint = { x, y }; }
    auto vanishingPoint() const { return m_vanishingPoint; }

    // Set the vertical field-of-view angle, in radians.
    void setFieldOfView(float radians);

    // Set the vertical field-of-view angle as a series of stops over zooms.
    void setFieldOfViewStops(std::shared_ptr<Stops> stops);

    // Get the vertical field-of-view angle, in radians.
    float getFieldOfView() const;

    // Set the vertical field-of-view angle to a value that corresponds to the
    // given focal length.
    void setFocalLength(float length);

    // Set the vertical field-of-view angle according to focal length as a
    // series of stops over zooms.
    void setFocalLengthStops(std::shared_ptr<Stops> stops);

    // Get the focal length that corresponds to the current vertical
    // field-of-view angle.
    float getFocalLength() const;

    // Set the minimum zoom level. Clamped to >=0.
    void setMinZoom(float minZoom);

    // Get the minimum zoom level.
    float getMinZoom() const { return m_minZoom; }

    // Set the maximum zoom level. Clamped to <= 20.5.
    void setMaxZoom(float maxZoom);

    // Get the maximum zoom level.
    float getMaxZoom() const { return m_maxZoom; }

    // Set the maximum pitch angle in degrees.
    void setMaxPitch(float degrees);

    // Set the maximum pitch angle in degrees as a series of stops over zooms.
    void setMaxPitchStops(std::shared_ptr<Stops> stops);

    // Get the maximum pitch angle for the current zoom, in degrees.
    float getMaxPitch() const;

    // Whether to constrain visible area to the projected bounds of the world.
    void setConstrainToWorldBounds(bool constrainToWorldBounds);

    // Set the ratio of hardware pixels to logical pixels. Default is 1.0.
    void setPixelScale(float _pixelsPerPoint);

    // Set the size of the viewable area in pixel.
    void setSize(int _width, int _height);

    // Set the position of the view within the world in projected meters.
    void setPosition(double _x, double _y);
    void setPosition(const glm::dvec2 pos) { setPosition(pos.x, pos.y); }

    void setCenterCoordinates(LngLat center);

    // Set the zoom level of the view.
    void setZoom(float _z);

    // Set the roll angle of the view in radians. Default is 0.
    void setRoll(float _rad);

    // Set the pitch angle of the view in radians. Default is 0.
    void setPitch(float _rad);

    // Move the position of the view in projected meters.
    void translate(double _dx, double _dy);

    // Change zoom by the given amount.
    void zoom(float _dz);

    // Change the roll angle by the given amount in radians.
    void roll(float _drad);

    // Change the pitch angle by the given amount in radians.
    void pitch(float _drad);

    // Get the current zoom.
    float getZoom() const { return m_zoom; }

    // Get the current zoom truncated to an integer. This is the zoom used to determine visible tiles.
    int getIntegerZoom() const { return static_cast<int>(m_zoom); }

    // Get the current roll angle in radians.
    float getRoll() const { return m_roll; }

    // Get the current pitch angle in radians.
    float getPitch() const { return m_pitch; }

    // Update the view and projection matrices if properties have changed.
    void update();

    // Get the position of the view in projection units (z is the effective 'height' determined from zoom).
    const glm::dvec3& getPosition() const { return m_pos; }

    // Get the coordinates of the point at the center of the view.
    LngLat getCenterCoordinates() const;

    // Get the transformation from global space into view (camera) space; Due to precision limits, this
    // does not contain the translation of the view from the global origin (you must apply that separately).
    const glm::mat4& getViewMatrix() const { return m_view; }

    // Get the transformation from view space into screen space.
    const glm::mat4& getProjectionMatrix() const { return m_proj; }

    // Get the combined view and projection transformation.
    const glm::mat4& getViewProjectionMatrix() const { return m_viewProj; }

    // Get the normal matrix; transforms surface normals from model space to camera space.
    const glm::mat3& getNormalMatrix() const { return m_normalMatrix; }

    const glm::mat3& getInverseNormalMatrix() const { return m_invNormalMatrix; }

    // Get the eye position in world space.
    const glm::vec3& getEye() const { return m_eye; }

    // Get the window coordinates [0,1], lower left corner of the window is (0, 0).
    glm::vec2 normalizedWindowCoordinates(float _x, float _y) const;

    ViewState state() const;

    // Get a rectangle of the current view range as [[x_min, y_min], [x_max, y_max]].
    glm::dmat2 getBoundsRect() const;

    float getWidth() const { return m_vpWidth; }

    float getHeight() const { return m_vpHeight; }

    EdgePadding getPadding() const { return m_padding; }

    void setPadding(EdgePadding padding);

    // Calculate the position on the ground plane (z = 0) under the given screen space coordinates,
    // replacing the input coordinates with world-space coordinates.
    // Returns the un-normalized distance 'into the screen' to the ground plane
    // (if < 0, intersection is behind the screen).
    double screenToGroundPlane(float& _screenX, float& _screenY);
    double screenToGroundPlane(double& _screenX, double& _screenY);

    // Get the screen position from a latitude/longitude.
    glm::vec2 lngLatToScreenPosition(double lng, double lat, bool& outsideViewport, bool clipToViewport = false);

    LngLat screenPositionToLngLat(float x, float y, bool& intersection);

    // For a position on the map in projected meters, this returns the displacement vector *from* the view *to* that
    // position, accounting for wrapping around the 180th meridian to get the smallest magnitude displacement.
    glm::dvec2 getRelativeMeters(glm::dvec2 projectedMeters) const;

    // Get the set of all tiles visible at the current position and zoom.
    void getVisibleTiles(const std::function<void(TileID)>& _tileCb) const;

    // Returns true if the view properties have changed since the last call to update().
    bool changedOnLastUpdate() const { return m_changed; }

    const glm::mat4& getOrthoViewportMatrix() const { return m_orthoViewport; };

    float pixelScale() const { return m_pixelScale; }
    float pixelsPerMeter() const;

    static float focalLengthToFieldOfView(float length);
    static float fieldOfViewToFocalLength(float radians);

protected:

    void updateMatrices();

    void applyWorldBounds();

    double screenToGroundPlaneInternal(double& _screenX, double& _screenY) const;

    std::shared_ptr<Stops> m_fovStops;
    std::shared_ptr<Stops> m_maxPitchStops;

    ViewConstraint m_constraint;

    glm::dvec3 m_pos;
    glm::vec3 m_eye;
    glm::vec2 m_obliqueAxis;
    glm::vec2 m_vanishingPoint;

    glm::mat4 m_view;
    glm::mat4 m_orthoViewport;
    glm::mat4 m_proj;
    glm::mat4 m_viewProj;
    glm::mat4 m_invViewProj;
    glm::mat3 m_normalMatrix;
    glm::mat3 m_invNormalMatrix;

    float m_roll = 0.f;
    float m_pitch = 0.f;

    float m_zoom = 0.f;
    float m_worldBoundsMinZoom = 0.f;

    float m_width;
    float m_height;

    int m_vpWidth;
    int m_vpHeight;
    float m_aspect;
    float m_pixelScale = 1.0f;
    float m_fov = 0.25 * PI;
    float m_maxPitch = 90.f;
    float m_minZoom = 0.f;
    float m_maxZoom = 20.5f;

    CameraType m_type;

    EdgePadding m_padding;

    bool m_dirtyMatrices;
    bool m_dirtyTiles;
    bool m_dirtyWorldBoundsMinZoom;
    bool m_changed;
    bool m_constrainToWorldBounds = true;

};

}
