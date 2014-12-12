#include "light.h"

#include "util/stringsOp.h"

Light::Light():m_name("not_define_light"),m_ambient(0.0f),m_diffuse(1.0f),m_specular(0.0f),m_index(-1),m_type(LIGHT_NOT_DEFINE){

}

Light::~Light(){

};

void Light::setName(const std::string &_name){
    m_name = _name;
}

void Light::setIndexPos(int _indexPos){
	m_index = _indexPos;
}

void Light::setAmbientColor(const glm::vec4 _ambient){ 
    m_ambient = _ambient;
}

void Light::setDiffuseColor(const glm::vec4 _diffuse){ 
    m_diffuse = _diffuse;
}

void Light::setSpecularColor(const glm::vec4 _specular){ 
    m_specular = _specular;
}

void Light::setupProgram( ShaderProgram &_shader ){
    _shader.setUniformf(getUniformName()+".ambient", m_ambient);
    _shader.setUniformf(getUniformName()+".diffuse", m_diffuse);
    _shader.setUniformf(getUniformName()+".specular", m_specular);
}

std::string Light::getName(){
	return m_name;
}

LightType Light::getType(){
    return m_type;
}

std::string Light::getUniformName(){
	if( m_index >= 0 ){
		return "u_" + m_name + "s["+getString(m_index)+"]";
	} else {
		return "u_" + m_name;
	}
}

std::string Light::getInstanceComputeBlock(){
    return "calculateLight("+getUniformName()+", eye, _ecPosition, _normal, amb, diff, spec);\n";
}
