#pragma once

#include "abstractLight.h"

class PointLight : public AbstractLight {
public:
    
    void setPosition(const glm::vec3 &_pos);
    
    virtual std::string getTransform();
    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec4 m_position;
};