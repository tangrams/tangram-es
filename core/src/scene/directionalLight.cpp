#include "scene/directionalLight.h"

#include "gl/shaderProgram.h"
#include "directionalLight_glsl.h"
#include "platform.h"
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

std::unique_ptr<LightUniforms> DirectionalLight::getUniforms() {

    if (!m_dynamic) { return nullptr; }

    return std::make_unique<Uniforms>(getUniformName());
}

void DirectionalLight::setupProgram(RenderState& rs, const View& _view, ShaderProgram& _shader,
                                    LightUniforms& _uniforms) {

    glm::vec3 direction = m_direction;
    if (m_origin == LightOrigin::world) {
        direction = _view.getNormalMatrix() * direction;
    }

    Light::setupProgram(rs, _view, _shader, _uniforms);

    auto& u = static_cast<DirectionalLight::Uniforms&>(_uniforms);
    _shader.setUniformf(rs, u.direction, direction);
}

std::string DirectionalLight::getClassBlock() {
    return SHADER_SOURCE(directionalLight_glsl);
}

std::string DirectionalLight::getInstanceDefinesBlock() {
    //	Directional lights don't have defines.... yet.
    return "\n";
}

std::string DirectionalLight::getInstanceAssignBlock() {
    std::string block = Light::getInstanceAssignBlock();
    if (!m_dynamic) {
        block += ", " + glm::to_string(m_direction) + ")";
    }
    return block;
}

const std::string& DirectionalLight::getTypeName() {

    return s_typeName;

}

}
