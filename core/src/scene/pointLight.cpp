#include "pointLight.h"

#define STRINGIFY(A) #A

void PointLight::setPosition(const glm::vec3 &_pos){
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 1.0;
}

void PointLight::setupProgram( ShaderProgram &_shader ){
    AbstractLight::setupProgram(_shader);
    _shader.setUniformf(getUniformName()+".position", m_position);
}

std::string PointLight::getTransform(){
    //return stringFromResource("modules/point_light.glsl");
    return stringFromResource("point_light.glsl");
}
