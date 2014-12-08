#include "abstractLight.h"

#include "util/stringsOp.h"

#define STRINGIFY(A) #A

AbstractLight::AbstractLight():m_name("abstractLight"),m_ambient(0.0f),m_diffuse(1.0f),m_specular(0.0f),m_index(-1){

}

AbstractLight::~AbstractLight(){

};

void AbstractLight::setName(const std::string &_name, int _indexPos){
    m_name = _name;

    if (_indexPos >= 0){
    	setIndexPos(_indexPos);
    }
}

void AbstractLight::setIndexPos(int _indexPos){
	m_index = _indexPos;
}

void AbstractLight::setAmbientColor(const glm::vec4 _ambient){ 
    m_ambient = _ambient;
}

void AbstractLight::setDiffuseColor(const glm::vec4 _diffuse){ 
    m_diffuse = _diffuse;
}

void AbstractLight::setSpecularColor(const glm::vec4 _specular){ 
    m_specular = _specular;
}

void AbstractLight::setupProgram( ShaderProgram &_shader ){
    _shader.setUniformf(getUniformName()+".ambient", m_ambient);
    _shader.setUniformf(getUniformName()+".diffuse", m_diffuse);
    _shader.setUniformf(getUniformName()+".specular", m_specular);
}

std::string AbstractLight::getName(){
	return m_name;
}

std::string AbstractLight::getUniformName(){
	if( m_index >= 0 ){
		return "u_" + m_name + "["+getString(m_index)+"]";
	} else {
		return "u_" + m_name;
	}
}