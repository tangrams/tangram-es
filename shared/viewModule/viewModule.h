#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <math.h>

/* ViewModule
 * 1. Stores a representation of the current view into the map world
 * 2. Determines which tiles are visible in the current view
 * 3. Tracks changes in the view state to determine when new rendering is needed
 *
 * TODO: Make this into an interface for different implementations
 * For now, this is a simple implementation of the viewModule responsibilities
 * using a top-down axis-aligned orthographic view
*/

#define EARTH_RADIUS_M 6378137.0
#define PI 3.1415926535

class ViewModule {

public:
	
	ViewModule(float width, float height);
	ViewModule();

	void setAspect(float width, float height);
	void setPosition(float x, float y);
	void setZoom(int z);
	void translate(float dx, float dy);
	void zoom(int dz);

	glm::vec3 getPosition() { return m_pos; };
	glm::mat4 getViewMatrix() { return m_view; };
	glm::mat4 getProjectionMatrix() { return m_proj; };

	glm::mat2 getBoundsRect(); // Returns a rectangle of the current view range as [[x_min, y_min][x_max, y_max]]

private:
	
	glm::vec3 m_pos;
	glm::mat4 m_view;
	glm::mat4 m_proj;
	int m_zoom;
	float m_width;
	float m_height;
	float m_aspect;

	void init(float width, float height);

};