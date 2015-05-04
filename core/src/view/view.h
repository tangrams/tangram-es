#pragma once

#include <vector>
#include <set>
#include <memory>

#include "glm/mat4x4.hpp"
#include "glm/vec4.hpp"
#include "glm/vec3.hpp"

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
    
    /* Sets a new map projection with default tileSize */
    void setMapProjection(ProjectionType _projType);
    
    /* Gets the current map projection */
    const MapProjection& getMapProjection() const;
    
    /* Sets the ratio of hardware pixels to logical pixels (for high-density screens)
     * 
     * If unset, default is 1.0 
     */
    void setPixelScale(float _pixelsPerPoint);

    /* Sets the size of the viewable area in pixels */
    void setSize(int _width, int _height);
    
    /* Sets the position of the view within the world (in projection units) */
    void setPosition(double _x, double _y);
    
    /* Sets the zoom level of the view */
    void setZoom(float _z);
    
    /* Sets the roll angle of the view in radians (default is 0) */
    void setRoll(float _rad);

    /* Sets the pitch angle of the view in radians (default is 0) */
    void setPitch(float _rad);
    
    /* Moves the position of the view */
    void translate(double _dx, double _dy);
    
    /* Changes zoom by the given amount */
    void zoom(float _dz);
    
    /* Changes the roll angle by the given amount in radians */
    void roll(float _drad);

    /* Changes the pitch angle by the given amount in radians */
    void pitch(float _drad);
    
    /* Rotates the view by the given amount in radians and translates the view such that 
       the given position on the ground plane remains stationary in screen space */
    void orbit(float _x, float _y, float _radians);
    
    /* Gets the current zoom */
    float getZoom() const { return m_zoom; }

	/* Get the current m_zoomIn */
	bool isZoomIn() const { return m_isZoomIn; }
    
    /* Get the current roll angle in radians */
    float getRoll() const { return m_roll; }
    
    /* Get the current pitch angle in radians */
    float getPitch() const { return m_pitch; }
    
    /* Updates the view and projection matrices if properties have changed */
    void update();
    
    /* Gets the position of the view in projection units (z is the effective 'height' determined from zoom) */
    const glm::dvec3& getPosition() const { return m_pos; }
    
    /* Gets the transformation from global space into view (camera) space; Due to precision limits, this 
       does not contain the translation of the view from the global origin (you must apply that separately) */
    const glm::mat4& getViewMatrix() const { return m_view; }

    /* Gets the transformation from view space into screen space */
    const glm::mat4& getProjectionMatrix() const { return m_proj; }

    /* Gets the combined view and projection transformation */
    const glm::mat4 getViewProjectionMatrix() const { return m_viewProj; }

    /* Gets the normal matrix; transforms surface normals from model space to camera space */
    const glm::mat3& getNormalMatrix() const { return m_normalMatrix; }

    /* Returns a rectangle of the current view range as [[x_min, y_min], [x_max, y_max]] */
    glm::dmat2 getBoundsRect() const;
    
    float getWidth() const { return m_vpWidth; }
    
    float getHeight() const { return m_vpHeight; }
    
    /* Calculate the position on the ground plane (z = 0) under the given screen space coordinates, 
       replacing the input coordinates with world-space coordinates */
    void screenToGroundPlane(float& _screenX, float& _screenY) const;
    
    /* Returns the set of all tiles visible at the current position and zoom */
    const std::set<TileID>& getVisibleTiles();
    
    /* Returns true if the view properties have changed since the last call to update() */
    bool changedOnLastUpdate() const { return m_changed; }

    virtual ~View() {
        m_visibleTiles.clear();
    }
    
    constexpr static float s_maxZoom = 18.0;

protected:
    
    void updateMatrices();
    void updateTiles();

    std::unique_ptr<MapProjection> m_projection;
    std::set<TileID> m_visibleTiles;

    glm::dvec3 m_pos;

    glm::mat4 m_view;
    glm::mat4 m_proj;
    glm::mat4 m_viewProj;
    glm::mat4 m_invViewProj;
    glm::mat3 m_normalMatrix;
    
    float m_roll = 0.f;
    float m_pitch = 0.f;
    
    float m_zoom;
    float m_initZoom = 16.0;
    bool m_isZoomIn = false;

    float m_width;
    float m_height;
    
    int m_vpWidth;
    int m_vpHeight;
    float m_aspect;
    float m_pixelScale = 1.0f;
    float m_pixelsPerTile = 256.0;

    bool m_dirty;
    bool m_changed;
    
};

