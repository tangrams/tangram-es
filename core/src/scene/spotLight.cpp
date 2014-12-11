#include "spotLight.h"


SpotLight::SpotLight():m_position(0.0),m_direction(1.0,0.0,0.0),m_constantAttenuation(0.0),m_linearAttenuation(0.0),m_quadraticAttenuation(0.0),m_spotExponent(0.0),m_spotCutoff(0.0),m_spotCosCutoff(0.0){
    m_name = "spotLight";
    m_type = LIGHT_SPOT;
}

SpotLight::~SpotLight(){

}

void SpotLight::setPosition(const glm::vec3 &_pos){
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 1.0;
}

void SpotLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void SpotLight::setAttenuation(float _constant, float _linear, float _quadratic){
    m_constantAttenuation = _constant;
    m_linearAttenuation = _linear;
    m_quadraticAttenuation = _quadratic;
}

void SpotLight::setCutOff(float _cutoff, float _exponent){
    m_spotCutoff = _cutoff;
    m_spotCosCutoff = cos(_cutoff);
    m_spotExponent = _exponent;
}

void SpotLight::setupProgram( ShaderProgram &_shader ){
    Light::setupProgram(_shader);
    _shader.setUniformf(getUniformName()+".position", m_position);
    _shader.setUniformf(getUniformName()+".direction", m_direction);

    if(m_constantAttenuation!=0.0){
        _shader.setUniformf(getUniformName()+".constantAttenuation", m_constantAttenuation);
    }

    if(m_linearAttenuation!=0.0){
        _shader.setUniformf(getUniformName()+".linearAttenuation", m_linearAttenuation);
    }

    if(m_quadraticAttenuation!=0.0){
        _shader.setUniformf(getUniformName()+".quadraticAttenuation", m_quadraticAttenuation);
    }

    // _shader.setUniformf(getUniformName()+".spotCutoff", m_spotCutoff);
    _shader.setUniformf(getUniformName()+".spotCosCutoff", m_spotCosCutoff);
    _shader.setUniformf(getUniformName()+".spotExponent", m_spotExponent);
}

std::string SpotLight::getDefinesBlock(){
        std::string defines = "\n";

    if(m_constantAttenuation!=0.0){
        defines += "#ifndef SPOTLIGHT_CONSTANT_ATTENUATION\n";
        defines += "#define SPOTLIGHT_CONSTANT_ATTENUATION\n";
        defines += "#endif\n\n";
    }

    if(m_linearAttenuation!=0.0){
        defines += "#ifndef SPOTLIGHT_LINEAR_ATTENUATION\n";
        defines += "#define SPOTLIGHT_LINEAR_ATTENUATION\n";
        defines += "#endif\n\n";
    }

    if(m_quadraticAttenuation!=0.0){
        defines += "#ifndef SPOTLIGHT_QUADRATIC_ATTENUATION\n";
        defines += "#define SPOTLIGHT_QUADRATIC_ATTENUATION\n";
        defines += "#endif\n\n";
    }
    return defines;
}

std::string SpotLight::getClassBlock(){
    return stringFromResource("spot_light.glsl");
}

std::string SpotLight::getBlock(){
    return getDefinesBlock() + "\n" + getClassBlock() + "\n";
}


