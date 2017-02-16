#include "scene/spotLight.h"

#include "gl/shaderProgram.h"
#include "platform.h"
#include "spotLight_glsl.h"
#include "view/view.h"

#include "glm/gtx/string_cast.hpp"

namespace Tangram {

std::string SpotLight::s_typeName = "SpotLight";

SpotLight::SpotLight(const std::string& _name, bool _dynamic) :
    PointLight(_name, _dynamic),
    m_direction(1.0,0.0,0.0),
    m_spotExponent(0.0),
    m_spotCutoff(0.0),
    m_spotCosCutoff(0.0) {

    m_type = LightType::spot;
}

SpotLight::~SpotLight() {}

void SpotLight::setDirection(const glm::vec3 &_dir) {
    m_direction = _dir;
}

void SpotLight::setCutoffAngle(float _cutoffAngle) {
    m_spotCutoff = _cutoffAngle;
    m_spotCosCutoff = cos(_cutoffAngle * 3.14159 / 180.0);
}

void SpotLight::setCutoffExponent(float _exponent) {
    m_spotExponent = _exponent;
}

std::unique_ptr<LightUniforms> SpotLight::getUniforms() {

    if (!m_dynamic) { return nullptr; }

    return std::make_unique<Uniforms>(getUniformName());
}

void SpotLight::setupProgram(RenderState& rs, const View& _view, ShaderProgram& _shader,
                             LightUniforms& _uniforms) {
    PointLight::setupProgram(rs, _view, _shader, _uniforms);

    glm::vec3 direction = m_direction;
    if (m_origin == LightOrigin::world) {
        direction = glm::normalize(_view.getNormalMatrix() * direction);
    }

    auto& u = static_cast<Uniforms&>(_uniforms);
    _shader.setUniformf(rs, u.direction, direction);
    _shader.setUniformf(rs, u.spotCosCutoff, m_spotCosCutoff);
    _shader.setUniformf(rs, u.spotExponent, m_spotExponent);
}

std::string SpotLight::getClassBlock() {
    return SHADER_SOURCE(spotLight_glsl);
}

std::string SpotLight::getInstanceAssignBlock() {
    std::string block = Light::getInstanceAssignBlock();

    if (!m_dynamic) {
        block += ", " + glm::to_string(m_position.value);
        if (m_attenuation!=0.0) {
            block += ", " + std::to_string(m_attenuation);
        }
        if (m_innerRadius!=0.0) {
            block += ", " + std::to_string(m_innerRadius);
        }
        if (m_outerRadius!=0.0) {
            block += ", " + std::to_string(m_outerRadius);
        }

        block += ", " + glm::to_string(m_direction);
        block += ", " + std::to_string(m_spotCosCutoff);
        block += ", " + std::to_string(m_spotExponent);

        block += ")";
    }
    return block;
}

const std::string& SpotLight::getTypeName() {

    return s_typeName;

}

}
