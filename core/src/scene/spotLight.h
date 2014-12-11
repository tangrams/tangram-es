#pragma once

#include "light.h"

class SpotLight : public Light {
public:
    
    SpotLight();
    virtual ~SpotLight();

    void setPosition(const glm::vec3 &_pos);
    void setDirection(const glm::vec3 &_dir);
    
    void setAttenuation(float _constant = 0.0, float _linear = 0.0, float _quadratic = 0.0);
    void setCutOff(float _cutoff, float _exponent);
    
    virtual std::string getDefinesBlock();
    static  std::string getClassBlock();

    virtual std::string getBlock();

    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec4 m_position;
   	glm::vec3 m_direction;

    float m_constantAttenuation;
    float m_linearAttenuation;
    float m_quadraticAttenuation;
    
   	float m_spotExponent;
    float m_spotCutoff;
    float m_spotCosCutoff;
};