#pragma once

#include "light.h"

class DirectionalLight : public Light {
public:
    
    DirectionalLight();
    virtual ~DirectionalLight();

    /*	Set the direction of the light */
    virtual void setDirection(const glm::vec3 &_dir);
    
    /*  GLSL #defines with the NUMBER of lights of this type */
    static std::string getArrayDefinesBlock(int _numberOfLights);

    /*  GLSL #defines with the NUMBER of lights of this type */
    static std::string getArrayUniformBlock();

    /*  GLSL block code with structs and need functions for this light type */
    static std::string getClassBlock();

    virtual std::string getInstanceDefinesBlock();
    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec3 m_direction;
};
