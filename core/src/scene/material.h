//
//  Based on openGL 2.0 GLSL gl_MaterialParameters
//  http://mew.cx/glsl_quickref.pdf
//
#pragma once

#include "glm/glm.hpp"

class Material {
public:
    
    Material();
    virtual ~Material(){};
        
    glm::vec4   m_emission;
    glm::vec4   m_ambient;
    glm::vec4   m_diffuse;
    glm::vec4   m_specular;
    
    float       m_shininess;
};