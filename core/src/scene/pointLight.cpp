#include "pointLight.h"

#define STRINGIFY(A) #A

void PointLight::setPosition(const glm::vec3 &_pos){
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 1.0;
}

void PointLight::setupProgram( ShaderProgram &_shader ){
    AbstractLight::setupProgram(_shader);
    
    _shader.setUniformf(getUniformName()+".position", m_position);
}

std::string PointLight::getTransform(){
    return STRINGIFY(

struct PointLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
   	vec4 position;
};

void calculatePointLight(in PointLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    // Compute vector from surface to light position
    vec3 VP = vec3(_light.position) - _ecPosition3;

    // Normalize the vector from surface to light position
    VP = normalize(VP);

    // Direction of maximum highlights
    vec3 halfVector = normalize(VP + _eye);

    float nDotVP = max(0.0, dot(_normal, VP));
    float nDotHV = max(0.0, dot(_normal, halfVector));

    float pf = 0.0;	// power factor for shinny speculars
    if (nDotVP != 0.0){
    	pf = pow(nDotHV, u_material.shininess);
    }
       
    _ambient += _light.ambient;
    _diffuse += _light.diffuse * nDotVP;
    _specular += _light.specular * pf;
} );

}
