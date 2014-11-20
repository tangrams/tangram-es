#include "light.h"

Light::Light(LightType _type):m_ambient(0.0f),m_diffuse(1.0f),m_specular(0.0f),m_position(0.0f),m_halfVector(0.0f),m_direction(0.0f),m_spotExponent(0.0f),m_spotCutoff(0.0f),m_spotCosCutoff(0.0f),m_constantAttenuation(0.0f),m_linearAttenuation(0.0f),m_quadraticAttenuation(0.0f),m_type(_type){
    
}

void Light::setAmbientColor(const glm::vec4 _ambient){
    m_ambient = _ambient;
}

void Light::setDiffuseColor(const glm::vec4 _diffuse){
    m_diffuse = _diffuse;
}

void Light::setSpecularColor(const glm::vec4 _specular){
    m_specular = _specular;
}

void Light::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void Light::setPosition(const float _lat, const float _lon, const float _alt){
    m_lat = _lat;
    m_lon = _lon;
    m_alt = _alt;
    
    //
    //
//    m_position = sphericalMercator(m_lat,m_lon,m_alt);
}