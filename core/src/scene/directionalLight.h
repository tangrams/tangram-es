#pragma once

#include "light.h"

class DirectionalLight : public Light {
public:
    
    DirectionalLight(const std::string& _name, bool _dynamic = false);
    virtual ~DirectionalLight();

    /*	Set the direction of the light */
    virtual void setDirection(const glm::vec3& _dir);
    
    virtual void setupProgram(std::shared_ptr<ShaderProgram> _program) override;
    
protected:

    /*  GLSL block code with structs and need functions for this light type */
    virtual std::string getClassBlock() override;
    virtual std::string getInstanceDefinesBlock() override;
    virtual std::string getInstanceAssignBlock() override;

    static std::string s_classBlock;
    
    glm::vec3 m_direction;
};
