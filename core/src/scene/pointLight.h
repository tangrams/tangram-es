#pragma once

#include "abstractLight.h"

class PointLight : public AbstractLight {
public:
    
    /* The position is relative to the camera */
    void setPosition(const glm::vec3 &_pos );
    
    virtual std::string getTransform();

    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec4	m_position;
};