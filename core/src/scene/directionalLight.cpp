#include "directionalLight.h"
#include "util/stringsOp.h"

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

void DirectionalLight::setupProgram( std::shared_ptr<ShaderProgram> _shader ) {
	if (m_dynamic) {
		Light::setupProgram(_shader);
		_shader->setUniformf(getUniformName()+".direction", m_direction);
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
		block += ", " + getString(m_direction) + ")";
	}
	return block;
}
