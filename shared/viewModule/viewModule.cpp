#include "viewModule.h"

ViewModule::ViewModule(float width, float height) {
	init(width, height);
}

ViewModule::ViewModule() {
	init(800, 600);
}

void ViewModule::init(float width, float height) {
	
	// Set up projection matrix based on input width and height with an arbitrary zoom
	setAspect(width, height);
	setZoom(16); // Arbitrary zoom for testing
	
	// Set up view matrix
	m_pos = glm::vec3(0, 0, 0); // Start at 0 to begin
	glm::vec3 direction = glm::vec3(0, 0, -1); // Look straight down
	glm::vec3 up = glm::vec3(0, 0, 1); // Z-axis is 'up'
	m_view = glm::lookAt(m_pos, m_pos + direction, up);

}

void ViewModule::setAspect(float width, float height) {

	m_aspect = width / height;

}

void ViewModule::setPosition(float x, float y) {
	
	translate(x - m_pos.x, y - m_pos.y);

}

void ViewModule::translate(float dx, float dy) {

	glm::translate(m_view, glm::vec3(dx, dy, 0.0));
	m_pos.x += dx;
	m_pos.y += dy;

}

void ViewModule::setZoom(int z) {

	m_zoom = z;
	float tileSize = 2 * PI * EARTH_RADIUS_M / pow(2, m_zoom);
	m_height = 3 * tileSize; // Set viewport size to ~3 tiles vertically
	m_width = m_height * m_aspect; // Size viewport width to match aspect ratio
	m_proj = glm::ortho(-m_width/2.0, m_width/2.0, -m_height/2.0, m_height/2.0);

}

glm::mat2 ViewModule::getBoundsRect() {

	float hw = m_width/2.0;
	float hh = m_height/2.0;
	return glm::mat2(m_pos.x - hw, m_pos.y - hh, m_pos.x + hw, m_pos.y + hh);

}
