#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <math.h>

#include "../util/projection.h"

/* ViewModule
 * 1. Stores a representation of the current view into the map world
 * 2. Determines which tiles are visible in the current view
 * 3. Tracks changes in the view state to determine when new rendering is needed
 *
 * TODO: Make this into an interface for different implementations
 * For now, this is a simple implementation of the viewModule responsibilities
 * using a top-down axis-aligned orthographic view
*/

class ViewModule {

public:

	ViewModule(float _width, float _height, ProjectionType _projType);
	ViewModule(float _width, float _height, ProjectionType _projType, int _tileSize);
	ViewModule();
    
    //Sets a new map projection with default tileSize
    void setMapProjection(ProjectionType _projType); 
    //Sets a new map projection with specified tileSize
    void setMapProjection(ProjectionType _projType, int _tileSize);

	void setAspect(float _width, float _height);
	void setPosition(float _x, float _y);
	void setZoom(int _z);
	void translate(float _dx, float _dy);
	void zoom(int _dz);

	int getZoom() { return m_zoom; };
	glm::vec3 getPosition() { return m_pos; };
	glm::mat4 getViewMatrix() { return m_view; };
	glm::mat4 getProjectionMatrix() { return m_proj; };

	glm::mat2 getBoundsRect(); // Returns a rectangle of the current view range as [[x_min, y_min][x_max, y_max]]
	const std::vector<glm::ivec3>& getVisibleTiles();

    virtual ~ViewModule() {
        m_visibleTiles.clear();
    }

private:

    std::unique_ptr<MapProjection> m_projection;
	bool m_dirty;
	std::vector<glm::ivec3> m_visibleTiles;
	glm::vec3 m_pos;
	glm::mat4 m_view;
	glm::mat4 m_proj;
	int m_zoom;
	float m_width;
	float m_height;
	float m_aspect;

	void init(float _width, float _height);

};
