#include "directionalLight.h"
#include "platform.h"

void DirectionalLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void DirectionalLight::setupProgram( ShaderProgram &_shader ){
    AbstractLight::setupProgram(_shader);
    _shader.setUniformf(getUniformName()+".direction", m_direction);
}

std::string DirectionalLight::getTransform(){
    // return stringFromResource("modules/directional_light.glsl");
    return stringFromResource("directional_light.glsl");
}