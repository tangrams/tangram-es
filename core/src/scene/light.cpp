#include "light.h"

#include "gl/shaderProgram.h"

#include "glm/gtx/string_cast.hpp"

namespace Tangram {

Light::Light(const std::string& _name, bool _dynamic):
    m_name(_name),
    m_ambient(0.0f),
    m_diffuse(1.0f),
    m_specular(0.0f),
    m_type(LightType::ambient),
    m_origin(LightOrigin::camera),
    m_dynamic(_dynamic) {
}

Light::~Light() {

}

void Light::setInstanceName(const std::string &_name) {
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

void Light::setOrigin(LightOrigin origin) {
    m_dynamic = true;
    m_origin = origin;
}

void Light::buildSetupBlock(ShaderSource& _out) {
    if (m_dynamic) {
        // If the light is dynamic, initialize it using the corresponding uniform at the start of main()
        _out << "    " + getInstanceName() + " = " + getUniformName() + ";";
    }
}

void Light::setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms) {
    _uniforms.shader.setUniformf(rs, _uniforms.ambient, m_ambient);
    _uniforms.shader.setUniformf(rs, _uniforms.diffuse, m_diffuse);
    _uniforms.shader.setUniformf(rs, _uniforms.specular, m_specular);
}

LightType Light::getType() {
    return m_type;
}

std::string Light::getUniformName() {
	return "u_" + m_name;
}

std::string Light::getInstanceName() {
    return m_name;
}

void Light::buildInstanceBlock(ShaderSource& _out) {
    const std::string& typeName = getTypeName();
    if (m_dynamic) {
        //  If is dynamic, define the uniform and copy it to the global instance of the light struct
        _out << "uniform " + typeName + " " + getUniformName() + ";";
        _out << typeName + " " + getInstanceName() + ";";
    } else {
        //  If is not dynamic define the global instance of the light struct and fill the variables
        _out += typeName + " " + getInstanceName() + " = " + getTypeName() + "(";
        buildInstanceAssignBlock(_out);
        _out << ");";
    }
}

void Light::buildInstanceAssignBlock(ShaderSource& _out) {
    if (!m_dynamic) {
        _out += "\n    " + glm::to_string(m_ambient);
        _out += ",\n    " + glm::to_string(m_diffuse);
        _out += ",\n    " + glm::to_string(m_specular);
    }
}

void Light::buildInstanceComputeBlock(ShaderSource& _out) {
    _out << "    calculateLight(" + getInstanceName() + ", _eyeToPoint, _normal);";
}

}
