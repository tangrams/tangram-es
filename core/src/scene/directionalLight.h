#pragma once

#include "light.h"

class DirectionalLight : public Light {
public:
    
    DirectionalLight(const std::string& _name, bool _dynamic = false);
    virtual ~DirectionalLight();

    /*	Set the direction of the light */
    virtual void setDirection(const glm::vec3& _dir);
    
    /*  GLSL block code with structs and need functions for this light type */
    static std::string getClassBlock();

    virtual std::string getInstanceDefinesBlock() override;
    virtual std::string	getInstanceAssignBlock() override;
    
    virtual void setupProgram( ShaderProgram* _program ) override;
    
protected:
    glm::vec3 m_direction;
};
