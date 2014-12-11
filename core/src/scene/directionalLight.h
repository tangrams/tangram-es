#pragma once

#include "light.h"

class DirectionalLight : public Light {
public:
    
    DirectionalLight();
    virtual ~DirectionalLight();

    virtual void setDirection(const glm::vec3 &_dir);
    
    static std::string getClassBlock();
    virtual std::string getDefinesBlock();

    virtual std::string getBlock();

    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec3 m_direction;
};
