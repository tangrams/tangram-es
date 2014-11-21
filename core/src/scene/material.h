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
    
    std::string getTransform();
    void        setupProgram( ShaderProgram &_shader );
    
    std::string m_name;
    
    glm::vec4   m_emission;
    glm::vec4   m_ambient;
    glm::vec4   m_diffuse;
    glm::vec4   m_specular;
    
    float       m_shininess;
};