//
//  Based on openGL 2.0 GLSL gl_MaterialParameters
//  http://mew.cx/glsl_quickref.pdf
//
#pragma once

#include "glm/glm.hpp"
#include "util/shaderProgram.h"

class Material {
public:
    
    Material();
    Material(const std::string &_name);
    
    virtual ~Material(){};
    
    void setName(const std::string &_name);

    std::string getName();
    std::string getUniformName();

    /* Inspired on Brett's webGL implementations get the glsl code to inject to define a Materials */
    std::string getTransform();

    /* Method to pass it self as a uniform to the shader program */
    void        setupProgram( ShaderProgram &_shader );
    
private:
    std::string m_name;
    
    glm::vec4   m_ambient;
    glm::vec4   m_diffuse;
    glm::vec4   m_specular;
    
    float       m_shininess;
};