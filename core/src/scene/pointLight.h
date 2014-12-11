#pragma once

#include "light.h"

class PointLight : public Light {
public:

	PointLight();
	virtual ~PointLight();
    
    /* The position is relative to the camera */
    void setPosition(const glm::vec3 &_pos );
    void setAttenuation(float _constant = 0.0, float _linear = 0.0, float _quadratic = 0.0);
    
    static std::string getClassBlock();
    virtual std::string getBlock();

    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec4	m_position;

    float m_constantAttenuation;
    float m_linearAttenuation;
    float m_quadraticAttenuation;
};