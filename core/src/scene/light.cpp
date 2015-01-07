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

    //  Each light will add the needed DEFINES
    _shader->addBlock("defines",    getInstanceDefinesBlock()); // DEFINES: depending what they need

    //  ... and depending the type of injection (vert, frag or both)
    if (m_injType == FRAGMENT ||
        m_injType == BOTH) {
        _shader->addBlock("frag_lighting",  getClassBlock() + // STRUCT and FUNCTION calculateLight (only once)
                                            getInstanceBlock() ); // INSTANCIATION of uniform (if dynamic) and global variable (width the data)
    }

    if (m_injType == VERTEX ||
        m_injType == BOTH) {
        _shader->addBlock("vert_lighting",  getClassBlock() + // STRUCT and FUNCTION calculateLight (only once)
                                            getInstanceBlock() ); // INSTANCIATION of uniform (if dynamic) and global variable (width the data)
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
    return "calculateLight("+getInstanceName()+", eye, _eyeToPoint, _normal, amb, diff, spec);\n";
}
