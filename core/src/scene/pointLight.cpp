#include "pointLight.h"

PointLight::PointLight():m_position(0.0),m_constantAttenuation(0.0),m_linearAttenuation(0.0),m_quadraticAttenuation(0.0){
    m_name = "pointLigth";
    m_type = LIGHT_POINT;
}

PointLight::~PointLight(){

}

void PointLight::setPosition(const glm::vec3 &_pos){
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 0.0;
}

void PointLight::setAttenuation(float _constant, float _linear, float _quadratic){
    m_constantAttenuation = _constant;
    m_linearAttenuation = _linear;
    m_quadraticAttenuation = _quadratic;
}

void PointLight::setupProgram( ShaderProgram &_shader ){
    Light::setupProgram(_shader);
    _shader.setUniformf(getUniformName()+".position", glm::vec4(m_position) );

    if(m_constantAttenuation!=0.0){
        _shader.setUniformf(getUniformName()+".constantAttenuation", m_constantAttenuation);
    }

    if(m_linearAttenuation!=0.0){
        _shader.setUniformf(getUniformName()+".linearAttenuation", m_linearAttenuation);
    }

    if(m_quadraticAttenuation!=0.0){
        _shader.setUniformf(getUniformName()+".quadraticAttenuation", m_quadraticAttenuation);
    }
}

std::string PointLight::getClassBlock(){
    return stringFromResource("point_light.glsl");
}

std::string PointLight::getBlock(){
    std::string defines = "\n";

    if(m_constantAttenuation!=0.0){
        defines += "#define POINTLIGHT_CONSTANT_ATTENUATION\n";
    }

    if(m_linearAttenuation!=0.0){
        defines += "#define POINTLIGHT_LINEAR_ATTENUATION\n";
    }

    if(m_quadraticAttenuation!=0.0){
        defines += "#define POINTLIGHT_QUADRATIC_ATTENUATION\n";
    }

    return defines + getClassBlock() + "\n";
}
