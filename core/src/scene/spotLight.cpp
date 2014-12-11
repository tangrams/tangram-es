#include "spotLight.h"


SpotLight::SpotLight():m_position(0.0),m_direction(1.0,0.0,0.0),m_spotExponent(0.0),m_spotCutoff(0.0),m_spotCosCutoff(0.0){
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

void SpotLight::setCutOff(float _cutoff, float _exponent){
    m_spotCutoff = _cutoff;
    m_spotCosCutoff = cos(_cutoff);
    m_spotExponent = _exponent;
}

void SpotLight::setupProgram( ShaderProgram &_shader ){
    Light::setupProgram(_shader);
    _shader.setUniformf(getUniformName()+".position", m_position);
    _shader.setUniformf(getUniformName()+".direction", m_direction);
    _shader.setUniformf(getUniformName()+".spotExponent", m_spotExponent);
    _shader.setUniformf(getUniformName()+".spotCutoff", m_spotCutoff);
    _shader.setUniformf(getUniformName()+".spotCosCutoff", m_spotCosCutoff);
}

std::string SpotLight::getDefinesBlock(){
    return "";
}

std::string SpotLight::getClassBlock(){
    return stringFromResource("spot_light.glsl");
}

std::string SpotLight::getBlock(){
    return getDefinesBlock() + "\n" + getClassBlock() + "\n";
}


