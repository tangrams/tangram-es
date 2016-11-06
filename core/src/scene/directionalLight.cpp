#include "directionalLight.h"

#include "gl/shaderProgram.h"
#include "style/material.h"
#include "view/view.h"

#include "glm/gtx/string_cast.hpp"

namespace Tangram {

std::string DirectionalLight::s_typeName = "DirectionalLight";

DirectionalLight::DirectionalLight(const std::string& _name, bool _dynamic) :
    Light(_name, _dynamic),
    m_direction(1.0,0.0,0.0) {

    m_type = LightType::directional;
}

DirectionalLight::~DirectionalLight() {}

void DirectionalLight::setDirection(const glm::vec3 &_dir) {
    m_direction = glm::normalize(_dir);
}

std::unique_ptr<LightUniforms> DirectionalLight::getUniforms(ShaderProgram& _shader) {

    if (m_dynamic) {
        return std::make_unique<Uniforms>(_shader, getUniformName());
    }
    return nullptr;
}

void DirectionalLight::setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms) {

    glm::vec3 direction = m_direction;
    if (m_origin == LightOrigin::world) {
        direction = _view.getNormalMatrix() * direction;
    }

    Light::setupProgram(rs, _view, _uniforms);

    auto& u = static_cast<DirectionalLight::Uniforms&>(_uniforms);
    u.shader.setUniformf(rs, u.direction, direction);
}

void DirectionalLight::buildClassBlock(Material& _material, ShaderSource& out) {

    out << "struct DirectionalLight {"
        << "    vec4 ambient;"
        << "    vec4 diffuse;"
        << "    vec4 specular;"
        << "    vec3 direction;"
        << "};";

    out << "void calculateLight(in DirectionalLight _light, in vec3 _eyeToPoint, in vec3 _normal) {"
        << "    light_accumulator_ambient += _light.ambient;";
    if (_material.hasDiffuse() || _material.hasSpecular()) {
        out << "    float nDotVP = clamp(dot(_normal, -_light.direction), 0.0, 1.0);";
    }
    if (_material.hasDiffuse()) {
        out << "    light_accumulator_diffuse += _light.diffuse * nDotVP;";
    }
    if (_material.hasSpecular()) {
        out << "    float pf = 0.0;"
            << "    if (nDotVP > 0.0) {"
            << "        vec3 reflectVector = reflect(_light.direction, _normal);"
            << "        float eyeDotR = max(dot(normalize(_eyeToPoint), reflectVector), 0.0);"
            << "        pf = pow(eyeDotR, material.shininess);"
            << "    }"
            << "    light_accumulator_specular += _light.specular * pf;";
    }
    out << "}";
}

void DirectionalLight::buildInstanceAssignBlock(ShaderSource& out) {
    Light::buildInstanceAssignBlock(out);
    if (!m_dynamic) {
        out += ",\n    " + glm::to_string(m_direction);
    }
}

const std::string& DirectionalLight::getTypeName() {

    return s_typeName;

}

}
