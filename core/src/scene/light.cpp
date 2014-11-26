#include "light.h"

#define STRINGIFY(A) #A

Light::Light():m_name("abstractLight"),m_ambient(0.0f),m_diffuse(1.0f),m_specular(0.0f){
}
void Light::setAmbientColor(const glm::vec4 _ambient){ m_ambient = _ambient;}
void Light::setDiffuseColor(const glm::vec4 _diffuse){ m_diffuse = _diffuse;}
void Light::setSpecularColor(const glm::vec4 _specular){ m_specular = _specular;}

void Light::setupProgram( ShaderProgram &_shader ){
    _shader.setUniformf("u_"+m_name+".ambient", m_ambient);
    _shader.setUniformf("u_"+m_name+".diffuse", m_diffuse);
    _shader.setUniformf("u_"+m_name+".specular", m_specular);
}

//  DIRECTIONAL LIGHT
//
void DirectionalLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

std::string DirectionalLight::getTransform(){
    return STRINGIFY(
                     struct DirectionalLight {
                         vec4 ambient;
                         vec4 diffuse;
                         vec4 specular;
                         
                         vec3 direction;
                     };
                     
                     void calculateDirectionalLight(in DirectionalLight _light, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
                         float nDotVP = max(0.0, dot(_normal, normalize(vec3(_light.direction))));
                         _ambient  += _light.ambient;
                         _diffuse  += _light.diffuse * nDotVP;
                     }
                     );
}

void DirectionalLight::setupProgram( ShaderProgram &_shader ){
    Light::setupProgram(_shader);
    
    _shader.setUniformf("u_"+m_name+".direction", m_direction);
}

//  POINT LIGHT
//
void PointLight::setPosition(const glm::vec3 &_pos){
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 1.0;
}

void PointLight::setAttenuation(float _constantAttenuation, float _linearAttenuation){
    m_constantAttenuation = _constantAttenuation;
    m_linearAttenuation = _linearAttenuation;
}

std::string PointLight::getTransform(){
    return STRINGIFY(
                     struct PointLight {
                         vec4 ambient;
                         vec4 diffuse;
                         vec4 specular;
                         
                         vec4 position;
                         
                         float constantAttenuation;
                         float linearAttenuation;
                     };
                     
                     void calculatePointLight(in PointLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
                         float nDotVP;
                         float nDotHV;
                         float pf;
                         float attenuation;
                         float d;
                         vec3  VP;
                         vec3  halfVector;
                         
                         VP = vec3(_light.position) - _ecPosition3;
                         
                         d = length(VP);
                         
                         VP = normalize(VP);
                         
                         attenuation = 1.0 / (_light.constantAttenuation +
                                              _light.linearAttenuation * d);
                         
                         halfVector = normalize(VP + _eye);
                         
                         nDotVP = max(0.0, dot(_normal, VP));
                         nDotHV = max(0.0, dot(_normal, halfVector));
                         
                         if (nDotVP == 0.0)
                             pf = 0.0;
                         else
                             pf = pow(nDotHV, u_material.shininess);
                         
                         _ambient += _light.ambient * attenuation;
                         _diffuse += _light.diffuse * nDotVP * attenuation;
                         _specular += _light.specular * pf * attenuation;
                     }
                     );
}

void PointLight::setupProgram( ShaderProgram &_shader ){
    Light::setupProgram(_shader);
    
    _shader.setUniformf("u_"+m_name+".position", m_position);
    _shader.setUniformf("u_"+m_name+".constantAttenuation", m_constantAttenuation);
    _shader.setUniformf("u_"+m_name+".linearAttenuation", m_linearAttenuation);
}

//  SPOT LIGHT
//
void SpotLight::setPosition(const glm::vec3 &_pos){
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 1.0;
}

void SpotLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void SpotLight::setAttenuation(float _attenuation){
    m_constantAttenuation = _attenuation;
}

void SpotLight::setCutOff(float _cutoff, float _exponent){
    m_spotCutoff = _cutoff;
    m_spotCosCutoff = cos(_cutoff);
    m_spotExponent = _exponent;
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
                         float constantAttenuation;
                     };
                     
                     void calculateSpotLight(in SpotLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
                         float nDotVP;           // normal . light direction
                         float nDotHV;           // normal . light half vector
                         float pf;               // power factor
                         float spotDot;          // cosine of angle between spotlight
                         float spotAttenuation;  // spotlight attenuation factor
                         float attenuation;      // computed attenuation factor
                         float d;                // distance from surface to light source
                         vec3 VP;                // direction from surface to light position
                         vec3 halfVector;        // direction of maximum highlights
                         
                         // Compute vector from surface to light position
                         VP = vec3(_light.position) - _ecPosition3;
                         
                         // Normalize the vector from surface to light position
                         VP = normalize(VP);
                         
                         // Compute attenuation
                         attenuation = 1.0 / (_light.constantAttenuation);
                         
                         // See if point on surface is inside cone of illumination
                         spotDot = dot(-VP, normalize(_light.direction));
                         
                         if (spotDot < _light.spotCosCutoff)
                             spotAttenuation = 0.0; // light adds no contribution
                         else
                             spotAttenuation = pow(spotDot, _light.spotExponent);
                         
                         // Combine the spotlight and distance attenuation.
                         attenuation *= spotAttenuation;
                         
                         halfVector = normalize(VP + _eye);
                         
                         nDotVP = max(0.0, dot(_normal, VP));
                         nDotHV = max(0.0, dot(_normal, halfVector));
                         
                         if (nDotVP == 0.0)
                             pf = 0.0;
                         else
                             pf = pow(nDotHV, u_material.shininess);
                         
                         _ambient  += _light.ambient * attenuation;
                         _diffuse  += _light.diffuse * nDotVP * attenuation;
                         _specular += _light.specular * pf * attenuation;
                     }
                     );
}

void SpotLight::setupProgram( ShaderProgram &_shader ){
    Light::setupProgram(_shader);
    _shader.setUniformf("u_"+m_name+".position", m_position);
    _shader.setUniformf("u_"+m_name+".direction", m_direction);
    _shader.setUniformf("u_"+m_name+".spotExponent", m_spotExponent);
    _shader.setUniformf("u_"+m_name+".spotCutoff", m_spotCutoff);
    _shader.setUniformf("u_"+m_name+".spotCosCutoff", m_spotCosCutoff);
    _shader.setUniformf("u_"+m_name+".constantAttenuation", m_constantAttenuation);
}