#include "light.h"

#include "util/stringsOp.h"

Light::Light(const std::string& _name, bool _dynamic):m_name(_name),m_typeName("undefined_light"),m_ambient(0.0f),m_diffuse(1.0f),m_specular(0.0f),m_type(LightType::UNDEFINED),m_injType(VERTEX),m_dynamic(_dynamic) {

}

Light::~Light() {

}

void Light::setName(const std::string &_name) {
    m_name = _name;
}

void Light::setAmbientColor(const glm::vec4 _ambient) {
    m_ambient = _ambient;
}

void Light::setDiffuseColor(const glm::vec4 _diffuse) {
    m_diffuse = _diffuse;
}

void Light::setSpecularColor(const glm::vec4 _specular) {
    m_specular = _specular;
}

void Light::injectOnProgram(std::shared_ptr<ShaderProgram> _shader, InjectionType _injType) {

    if (_injType != DEFAULT) {
        m_injType = _injType;
    }
    
    _shader->addSourceBlock("defines", getInstanceDefinesBlock(), false);

    if (m_injType == FRAGMENT || m_injType == BOTH) {
        _shader->addSourceBlock("_fragment_lighting", getInstanceBlock());
        _shader->addSourceBlock("_fragment_lighting", getClassBlock(), false);
        _shader->addSourceBlock("fragment_lights_to_compute", getInstanceComputeBlock());

        //  TODO:
        //      - AM - After global blocks is implemented
        //      - AM - Implement defines (and use them on the lights and materials defines)
        //      - AM - Add a method to scene that add the main calcualtion function
        //              from scene you pass the light to this functions Light::getFragmentLightMainFunctionBlock() 
        //              and Light::getFVertexLightMainFunctionBlock()
    }

    if (m_injType == VERTEX || m_injType == BOTH) {
        _shader->addSourceBlock("_vertex_lighting", getInstanceBlock());
        _shader->addSourceBlock("_vertex_lighting", getClassBlock(), false);
        _shader->addSourceBlock("vertex_lights_to_compute", getInstanceComputeBlock());
    }
}

void Light::setupProgram(std::shared_ptr<ShaderProgram> _shader) {
    if (m_dynamic) {
        _shader->setUniformf(getUniformName()+".ambient", m_ambient);
        _shader->setUniformf(getUniformName()+".diffuse", m_diffuse);
        _shader->setUniformf(getUniformName()+".specular", m_specular);
    }
}

std::string Light::getName() {
	return m_name;
}

LightType Light::getType() {
    return m_type;
}

InjectionType Light::getInjectionType() {
    return m_injType;
}

std::string Light::getUniformName() {
	return "u_" + m_name;
}

std::string Light::getInstanceName() {
    return "g_" + m_name;
}

std::string Light::getInstanceBlock() {
    std::string block = "";
    if (m_dynamic) {
        //  If is dynamic, define the uniform and copy it to the global instance of the light struct
        block += "uniform " + m_typeName + " " + getUniformName() + ";\n";
        block += m_typeName + " " + getInstanceName() + " = " + getUniformName() + ";\n";
    } else {
        //  If is not dynamic define the global instance of the light struct and fill the variables
        block += m_typeName + " " + getInstanceName() + getInstanceAssignBlock() +";\n";
    }
    return block;
}

std::string Light::getInstanceAssignBlock() {
    std::string block = "";
    if (!m_dynamic) {
        block += " = " + m_typeName + "(" + getString(m_ambient);
        block += ", " + getString(m_diffuse);
        block += ", " + getString(m_specular);
    }
    return block;
}

std::string Light::getInstanceComputeBlock() {
    return "calculateLight("+getInstanceName()+", eye, _eyeToPoint, _normal);\n";
}
