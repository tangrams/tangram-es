#include "directionalLight.h"
#include "platform.h"

DirectionalLight::DirectionalLight():m_direction(1.0,0.0,0.0){
	m_name = "directionalLight";
	m_type = LIGHT_DIRECTIONAL;
}

DirectionalLight::~DirectionalLight(){

}

void DirectionalLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void DirectionalLight::setupProgram( ShaderProgram &_shader ){
    Light::setupProgram(_shader);
    _shader.setUniformf(getUniformName()+".direction", m_direction);
}

std::string DirectionalLight::getBlock(){
    return stringFromResource("directional_light.glsl");
}