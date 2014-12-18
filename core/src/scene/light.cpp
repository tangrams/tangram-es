#include "light.h"

#include "util/stringsOp.h"

Light::Light(const std::string& _name, bool _dynamic):m_name(_name),m_typeName("not_define_light"),m_ambient(0.0f),m_diffuse(1.0f),m_specular(0.0f),m_type(LightType::LIGHT_NOT_DEFINE),m_dynamic(_dynamic){

}

Light::~Light(){

};

void Light::setName(const std::string &_name){
    m_name = _name;
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

void Light::setupProgram( std::shared_ptr<ShaderProgram> _shader ){
    if(m_dynamic){
        _shader->setUniformf(getUniformName()+".ambient", m_ambient);
        _shader->setUniformf(getUniformName()+".diffuse", m_diffuse);
        _shader->setUniformf(getUniformName()+".specular", m_specular);
    }
}

std::string Light::getName(){
	return m_name;
}

LightType Light::getType(){
    return m_type;
}

std::string Light::getUniformName(){
	return "u_" + m_name;
}

std::string Light::getInstanceName(){
    return "g_" + m_name;
}

std::string Light::getInstanceBlock(){
    std::string block = "";
    if(m_dynamic){    
        //  If is dynamic, define the uniform and copy it to the global instance of the light struct
        block += "uniform " + m_typeName + " " + getUniformName() + ";\n";
        block += m_typeName + " " + getInstanceName() + " = " + getUniformName() + ";\n";
    } else {
        //  If is not dynamic define the global instance of the light struct and fill the variables
        // block += m_typeName + " " + getInstanceName() + ";\n";
        block += m_typeName + " " + getInstanceName() + getInstanceAssignBlock() +";\n";
    }
    return block;
}

std::string Light::getInstanceAssignBlock(){
    std::string block = "";
    if(!m_dynamic){
        // block += getInstanceName() + ".ambient = " + getString(m_ambient) + ";\n";
        // block += getInstanceName() + ".diffuse = " + getString(m_diffuse) + ";\n";
        // block += getInstanceName() + ".specular = " + getString(m_specular) + ";\n";

        block += " = " + m_typeName + "(" + getString(m_ambient);
        block += ", " + getString(m_diffuse);
        block += ", " + getString(m_specular);
    }
    return block;
}

std::string Light::getInstanceComputeBlock(){
    return "calculateLight("+getInstanceName()+", eye, _eyeToPoint, _normal, amb, diff, spec);\n";
}
