#include "material.h"

#define STRINGIFY(A) #A

Material::Material():m_name("material"),m_ambient(1.0),m_diffuse(1.0),m_specular(0.2),m_shininess(0.2){
    
}

Material::Material(const std::string &_name):m_name(_name),m_ambient(0.2),m_diffuse(0.8),m_specular(0.2),m_shininess(0.2){
    
}

std::string Material::getTransform(){
    return stringFromResource("modules/material.glsl");
}

void Material::setupProgram(ShaderProgram &_shader){
    _shader.setUniformf("u_"+m_name+".ambient", m_ambient);
    _shader.setUniformf("u_"+m_name+".diffuse", m_diffuse);
    _shader.setUniformf("u_"+m_name+".specular", m_specular);
    _shader.setUniformf("u_"+m_name+".shininess", m_shininess);
}