#include "directionalLight.h"
#include "util/stringsOp.h"

DirectionalLight::DirectionalLight(const std::string& _name, bool _dynamic):Light(_name,_dynamic),m_direction(1.0,0.0,0.0){
	m_typeName = "DirectionalLight";
	m_type = LightType::LIGHT_DIRECTIONAL;
}

DirectionalLight::~DirectionalLight(){

}

void DirectionalLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void DirectionalLight::setupProgram( ShaderProgram* _shader ){
	if(m_dynamic){
		Light::setupProgram(_shader);
    	_shader->setUniformf(getUniformName()+".direction", m_direction);
	}
}

std::string DirectionalLight::getClassBlock(){
    return stringFromResource("directional_light.glsl")+"\n";
}

std::string DirectionalLight::getInstanceDefinesBlock(){
	return "\n";
}

std::string DirectionalLight::getInstanceAssignBlock(){
    std::string block = Light::getInstanceAssignBlock();
    if(!m_dynamic){
        block += getInstanceName() + ".direction = " + getString(m_direction) + ";\n";
    }
    return block;
}
