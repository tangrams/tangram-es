#include "scene/ambientLight.h"

#include "gl/shaderProgram.h"
#include "ambientLight_glsl.h"

#include "glm/gtx/string_cast.hpp"

namespace Tangram {

std::string AmbientLight::s_typeName = "AmbientLight";

AmbientLight::AmbientLight(const std::string& _name, bool _dynamic) :
    Light(_name, _dynamic) {

    m_type = LightType::ambient;

}

AmbientLight::~AmbientLight() {}

std::unique_ptr<LightUniforms> AmbientLight::getUniforms() {

    if (!m_dynamic) { return nullptr; }

    return std::make_unique<LightUniforms>(getUniformName());
}

void AmbientLight::setupProgram(RenderState& rs, const View& _view, ShaderProgram& _shader,
                                LightUniforms& _uniforms) {
    Light::setupProgram(rs, _view, _shader, _uniforms);
}

std::string AmbientLight::getClassBlock() {
    return SHADER_SOURCE(ambientLight_glsl);
}

std::string AmbientLight::getInstanceDefinesBlock() {
    //  Ambient lights don't have defines.... yet.
    return "\n";
}

std::string AmbientLight::getInstanceAssignBlock() {
    std::string block = Light::getInstanceAssignBlock();
    if (!m_dynamic) {
        block += ")";
    }
    return block;
}

const std::string& AmbientLight::getTypeName() {

    return s_typeName;

}

}
