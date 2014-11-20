//
//  Based on openGL 2.0 GLSL gl_LightSourceParameters
//  http://mew.cx/glsl_quickref.pdf
//
#pragma once

#include <string>

#include "glm/glm.hpp"

enum LightType {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT
};

class Light {
public:
    
    Light(LightType _type);
    virtual ~Light(){};
    
    void setPosition(const float _lat, const float _lon, const float _alt);
    void setDirection(const glm::vec3 &_dir);
    void setCutOff(const float &_cutOff);
    
    void setAmbientColor(const glm::vec4 _ambient);
    void setDiffuseColor(const glm::vec4 _diffuse);
    void setSpecularColor(const glm::vec4 _specular);
    
    glm::vec4 m_ambient;
    glm::vec4 m_diffuse;
    glm::vec4 m_specular;
    glm::vec4 m_position;
    glm::vec4 m_halfVector;
    glm::vec3 m_direction;
    
    float m_spotExponent;
    float m_spotCutoff;
    float m_spotCosCutoff;
    float m_constantAttenuation;
    float m_linearAttenuation;
    float m_quadraticAttenuation;
    
    float m_lat;
    float m_lon;
    float m_alt;
    
    LightType   m_type;
};