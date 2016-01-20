#include "directionalLight.h"

#include "glm/gtx/string_cast.hpp"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "view/view.h"

namespace Tangram {

std::string DirectionalLight::s_classBlock;
std::string DirectionalLight::s_typeName = "DirectionalLight";

DirectionalLight::DirectionalLight(const std::string& _name, bool _dynamic) :
    Light(_name, _dynamic),
    m_direction(1.0,0.0,0.0),
    m_uDirection(getUniformName()+".direction") {

    m_type = LightType::directional;
}

DirectionalLight::~DirectionalLight() {

}

void DirectionalLight::setDirection(const glm::vec3 &_dir) {
    m_direction = glm::normalize(_dir);
}

void DirectionalLight::setupProgram(const View& _view, ShaderProgram& _shader ) {

    glm::vec3 direction = m_direction;
    if (m_origin == LightOrigin::world) {
        direction = _view.getNormalMatrix() * direction;
    }

	if (m_dynamic) {
		Light::setupProgram(_view, _shader);
		_shader.setUniformf(m_uDirection, direction);
	}
}

std::string DirectionalLight::getClassBlock() {
    if (s_classBlock.empty()) {
        s_classBlock = stringFromFile("shaders/directionalLight.glsl", PathType::internal)+"\n";
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

}
