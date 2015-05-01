#include "directionalLight.h"
#include "glm/gtx/string_cast.hpp"

std::string DirectionalLight::s_classBlock;

DirectionalLight::DirectionalLight(const std::string& _name, bool _dynamic):Light(_name,_dynamic),m_direction(1.0,0.0,0.0) {
	m_typeName = "DirectionalLight";
	m_type = LightType::DIRECTIONAL;
}

DirectionalLight::~DirectionalLight() {

}

void DirectionalLight::setDirection(const glm::vec3 &_dir) {
    m_direction = glm::normalize(_dir);
}

void DirectionalLight::setupProgram(const std::shared_ptr<View>& _view, std::shared_ptr<ShaderProgram> _shader ) {

    m_direction_eye = m_direction;
    if (m_origin != LightOrigin::CAMERA) {
        glm::mat3 normalMatrix = glm::mat3(_view->getViewMatrix()); // Transforms surface normals into camera space
        normalMatrix = glm::transpose(glm::inverse(normalMatrix));

        m_direction_eye = normalMatrix * m_direction;
    }

	if (m_dynamic) {
		Light::setupProgram(_view, _shader);
		_shader->setUniformf(getUniformName()+".direction", m_direction_eye);
	}
}

std::string DirectionalLight::getClassBlock() {
    if (s_classBlock.empty()) {
        s_classBlock = stringFromResource("directionalLight.glsl")+"\n";
    }
    return s_classBlock;
}

std::string DirectionalLight::getInstanceDefinesBlock() {
	//	Directional lights don't have defines.... yet.
	return "\n";
}

std::string DirectionalLight::getInstanceAssignBlock() {
	std::string block = Light::getInstanceAssignBlock();
	if (!m_dynamic) {
        block += ", " + glm::to_string(m_direction) + ")";
	}
	return block;
}
