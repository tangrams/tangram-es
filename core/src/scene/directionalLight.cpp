#include "directionalLight.h"

#define STRINGIFY(A) #A

void DirectionalLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void DirectionalLight::setupProgram( ShaderProgram &_shader ){
    AbstractLight::setupProgram(_shader);
    
    _shader.setUniformf(getUniformName()+".direction", m_direction);
}

std::string DirectionalLight::getTransform(){

    /* 
     *  Width an extra vec3 we can pass a half-vector direction to compute a better specular lit 
     */

    return STRINGIFY(
struct DirectionalLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec3 direction;
};

void calculateDirectionalLight(in DirectionalLight _light, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    float nDotVP    =  max(0.0,dot(_normal,normalize(vec3(_light.direction))));

    _ambient    += _light.ambient;
    _diffuse    += _light.diffuse * nDotVP;
    _specular   += _light.specular * nDotVP;
});

}