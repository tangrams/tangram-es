#include "ambientLight.h"

#include "gl/shaderProgram.h"

#include "glm/gtx/string_cast.hpp"

namespace Tangram {

std::string AmbientLight::s_typeName = "AmbientLight";

AmbientLight::AmbientLight(const std::string& _name, bool _dynamic) :
    Light(_name, _dynamic) {

    m_type = LightType::ambient;

}

AmbientLight::~AmbientLight() {}

std::unique_ptr<LightUniforms> AmbientLight::getUniforms(ShaderProgram& _shader) {

    if (m_dynamic) {
        return std::make_unique<LightUniforms>(_shader, getUniformName());
    }

    return nullptr;
}

void AmbientLight::setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms) {
    Light::setupProgram(rs, _view, _uniforms);
}

void AmbientLight::buildClassBlock(Material& _material, ShaderSource& out) {

    out << "struct AmbientLight {"
        << "    vec4 ambient;"
        << "    vec4 diffuse;"
        << "    vec4 specular;"
        << "};"
        << "void calculateLight(in AmbientLight _light, in vec3 _eyeToPoint, in vec3 _normal) {"
        << "    light_accumulator_ambient += _light.ambient;"
        << "}";
}

const std::string& AmbientLight::getTypeName() {

    return s_typeName;

}

}
