#pragma once

#include "light.h"

class SpotLight : public Light {
public:
    
    SpotLight();
    virtual ~SpotLight();

    void setPosition(const glm::vec3 &_pos);
    void setDirection(const glm::vec3 &_dir);
    
    void setCutOff(float _cutoff, float _exponent);
    
    static  std::string getBlock();
    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec4 m_position;
   	glm::vec3 m_direction;
    
   	float m_spotExponent;
    float m_spotCutoff;
    float m_spotCosCutoff;
};