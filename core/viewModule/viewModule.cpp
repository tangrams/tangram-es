#include "viewModule.h"

ViewModule::ViewModule(float _width, float _height) {
	init(_width, _height);
}

ViewModule::ViewModule() {
	init(800, 600);
}

void ViewModule::init(float _width, float _height) {

	m_dirty = true;

	// Set up projection matrix based on input width and height with an arbitrary zoom
	setAspect(_width, _height);
	setZoom(16); // Arbitrary zoom for testing

	// Set up view matrix
	m_pos = glm::vec3(0, 0, 0); // Start at 0 to begin
	glm::vec3 direction = glm::vec3(0, 0, -1); // Look straight down
	glm::vec3 up = glm::vec3(0, 0, 1); // Z-axis is 'up'
	m_view = glm::lookAt(m_pos, m_pos + direction, up);

}

void ViewModule::setAspect(float _width, float _height) {

	m_aspect = _width / _height;
	setZoom(m_zoom);
	m_dirty = true;

}

void ViewModule::setPosition(float _x, float _y) {

	translate(_x - m_pos.x, _y - m_pos.y);
	m_dirty = true;

}

void ViewModule::translate(float _dx, float _dy) {

	glm::translate(m_view, glm::vec3(_dx, _dy, 0.0));
	m_pos.x += _dx;
	m_pos.y += _dy;
	m_dirty = true;

}

void ViewModule::setZoom(int _z) {

	m_zoom = _z;
	float tileSize = 2 * PI * EARTH_RADIUS_M * pow(2, -m_zoom);
	m_height = 3 * tileSize; // Set viewport size to ~3 tiles vertically
	m_width = m_height * m_aspect; // Size viewport width to match aspect ratio
	m_proj = glm::ortho(-m_width * 0.5, m_width * 0.5, -m_height * 0.5, m_height * 0.5);
	m_dirty = true;

}

glm::mat2 ViewModule::getBoundsRect() {

	float hw = m_width * 0.5;
	float hh = m_height * 0.5;
	return glm::mat2(m_pos.x - hw, m_pos.y - hh, m_pos.x + hw, m_pos.y + hh);

}

const std::vector<glm::ivec3>& ViewModule::getVisibleTiles() {

	if (!m_dirty) {
		return m_visibleTiles;
	}

	m_visibleTiles.clear();

	float tileSize = 2 * PI * EARTH_RADIUS_M * pow(2, -m_zoom);
	float invTileSize = 1.0 / tileSize;

	float vpLeftEdge = m_pos.x - m_width * 0.5;
	float vpRightEdge = vpLeftEdge + m_width;
	float vpBottomEdge = m_pos.y - m_height * 0.5;
	float vpTopEdge = vpBottomEdge + m_height;

	int tileX = (int) vpLeftEdge * invTileSize;
	int tileY = (int) vpBottomEdge * invTileSize;

	float x = tileX * tileSize;
	float y = tileY * tileSize;

	while (x < vpRightEdge) {

		while (y < vpTopEdge) {

			m_visibleTiles.push_back(glm::ivec3(tileX, tileY, m_zoom));
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
