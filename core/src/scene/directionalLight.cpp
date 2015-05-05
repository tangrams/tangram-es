#include "directionalLight.h"
#include "glm/gtx/string_cast.hpp"

std::string DirectionalLight::s_classBlock;
std::string DirectionalLight::s_typeName = "DirectionalLight";

DirectionalLight::DirectionalLight(const std::string& _name, bool _dynamic) : 
    Light(_name, _dynamic),
    m_direction(1.0,0.0,0.0) {

    m_type = LightType::DIRECTIONAL;
}

DirectionalLight::~DirectionalLight() {

}

void DirectionalLight::setDirection(const glm::vec3 &_dir) {
    m_direction = glm::normalize(_dir);
}

void DirectionalLight::setupProgram(const std::shared_ptr<View>& _view, std::shared_ptr<ShaderProgram> _shader ) {

    glm::vec3 direction = m_direction;
    if (m_origin == LightOrigin::WORLD) {
        direction = _view->getNormalMatrix() * direction;
    }

	if (m_dynamic) {
		Light::setupProgram(_view, _shader);
		_shader->setUniformf(getUniformName()+".direction", direction);
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

const std::string& DirectionalLight::getTypeName() {

    return s_typeName;

}
