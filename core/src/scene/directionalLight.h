#pragma once

#include "abstractLight.h"

class DirectionalLight : public AbstractLight {
public:
    
    void setDirection(const glm::vec3 &_dir);
    
    //	TODO:
    // 			- RETHINK name
    //
    // std::string getGLSLCode();
    // std::string getPragmaCode();
    // std::string getShaderBlock();
    // std::string getShaderBlock();
    //
    virtual std::string getTransform();
    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec3 m_direction;
};
