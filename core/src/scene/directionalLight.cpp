#include "directionalLight.h"
#include "util/stringsOp.h"

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

std::string DirectionalLight::getArrayDefinesBlock(int _numberOfLights){
	return "#define NUM_DIRECTIONAL_LIGHTS " + getString(_numberOfLights) + "\n";
}

std::string DirectionalLight::getArrayUniformBlock(){
	return "uniform DirectionalLight u_directionalLights[NUM_DIRECTIONAL_LIGHTS];\n";
}

std::string DirectionalLight::getClassBlock(){
    return stringFromResource("directional_light.glsl")+"\n";
}

std::string DirectionalLight::getInstanceDefinesBlock(){
	return "\n";
}
