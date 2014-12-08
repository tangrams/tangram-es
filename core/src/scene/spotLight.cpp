#include "spotLight.h"

#define STRINGIFY(A) #A

void SpotLight::setPosition(const glm::vec3 &_pos){
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 1.0;
}

void SpotLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void SpotLight::setCutOff(float _cutoff, float _exponent){
    m_spotCutoff = _cutoff;
    m_spotCosCutoff = cos(_cutoff);
    m_spotExponent = _exponent;
}

void SpotLight::setupProgram( ShaderProgram &_shader ){
    AbstractLight::setupProgram(_shader);
    _shader.setUniformf(getUniformName()+".position", m_position);
    _shader.setUniformf(getUniformName()+".direction", m_direction);
    _shader.setUniformf(getUniformName()+".spotExponent", m_spotExponent);
    _shader.setUniformf(getUniformName()+".spotCutoff", m_spotCutoff);
    _shader.setUniformf(getUniformName()+".spotCosCutoff", m_spotCosCutoff);
}

std::string SpotLight::getTransform(){
    return STRINGIFY(

struct SpotLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 position;

    vec3 direction;

    float spotExponent;
    float spotCutoff;
    float spotCosCutoff;
};

void calculateSpotLight(in SpotLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    // Compute vector from surface to light position
    vec3 VP = vec3(_light.position) - _ecPosition3;

    // Normalize the vector from surface to light position
    VP = normalize(VP);

    // See if point on surface is inside cone of illumination
    float spotDot = dot(-VP, normalize(_light.direction));

    // spotlight attenuation factor
    float spotAttenuation = 0.0;
    if (spotDot >= _light.spotCosCutoff)
        spotAttenuation = pow(spotDot, _light.spotExponent);

    // Direction of maximum highlights
    vec3 halfVector = normalize(VP + _eye);

    // normal . light direction
    float nDotVP = max(0.0, dot(_normal, VP));

    // normal . light half vector
    float nDotHV = max(0.0, dot(_normal, halfVector));

    // Power factor for shinny speculars
    float pf = 0.0;              
    if (nDotVP != 0.0)
        pf = pow(nDotHV, u_material.shininess);

    _ambient  += _light.ambient * spotAttenuation;
    _diffuse  += _light.diffuse * nDotVP * spotAttenuation;
    _specular += _light.specular * pf * spotAttenuation;
} );
}
