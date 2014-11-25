//
//  Based on openGL 2.0 GLSL gl_LightSourceParameters
//  http://mew.cx/glsl_quickref.pdf
//
#pragma once

#include "glm/glm.hpp"

#include "util/shaderProgram.h"

class Light {
public:
    
    Light();
    virtual ~Light(){};
    
    void setAmbientColor(const glm::vec4 _ambient);
    void setDiffuseColor(const glm::vec4 _diffuse);
    void setSpecularColor(const glm::vec4 _specular);
    
    virtual std::string getTransform() = 0;
    virtual void setupProgram( ShaderProgram &_shader );
    
    std::string m_name;
    
protected:
    glm::vec4 m_ambient;
    glm::vec4 m_diffuse;
    glm::vec4 m_specular;
};

class DirectionalLight : public Light {
public:
    
    void setDirection(const glm::vec3 &_dir);
    
    virtual std::string getTransform();
    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec3 m_direction;
//    glm::vec3 m_halfVector;
};

class PointLight : public Light {
public:
    
    void setPosition(const glm::vec3 &_pos);
    void setAttenuation(float _attenuation);
    
    virtual std::string getTransform();
    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec4 m_position;
    
    float m_constantAttenuation;
//    float m_linearAttenuation;
//    float m_quadraticAttenuation;
};

class SpotLight : public Light {
public:
    
    void setPosition(const glm::vec3 &_pos);
    void setDirection(const glm::vec3 &_dir);
    void setAttenuation(float _attenuation);
    void setCutOff(float _cutoff, float _exponent);
    
    virtual std::string getTransform();
    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec4 m_position;
   	glm::vec3 m_direction;
    
   	float m_spotExponent;
    float m_spotCutoff;
    float m_spotCosCutoff;
    
    float m_constantAttenuation;
//    float m_linearAttenuation;
//    float m_quadraticAttenuation;
};
