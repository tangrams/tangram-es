#include "spotLight.h"

#include "glm/gtx/string_cast.hpp"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "view/view.h"

namespace Tangram {

std::string SpotLight::s_classBlock;
std::string SpotLight::s_typeName = "SpotLight";

SpotLight::SpotLight(const std::string& _name, bool _dynamic) :
    PointLight(_name, _dynamic),
    m_direction(1.0,0.0,0.0),
    m_spotExponent(0.0),
    m_spotCutoff(0.0),
    m_spotCosCutoff(0.0) {

    m_type = LightType::spot;

}

SpotLight::~SpotLight() {

}

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

void SpotLight::setupProgram(const View& _view, ShaderProgram& _shader ) {
    if (m_dynamic) {
        PointLight::setupProgram(_view, _shader);

        glm::vec3 direction = m_direction;
        if (m_origin == LightOrigin::world) {
            direction = glm::normalize(_view.getNormalMatrix() * direction);
        }

        if (m_directionUniform == 0) {
            UniformEntries::lazyGenEntry(&m_directionUniform, getUniformName() + ".direction");
        }

        if (m_spotCosCutoffUniform == 0) {
            UniformEntries::lazyGenEntry(&m_spotCosCutoffUniform, getUniformName() + ".spotCosCutoff");
        }

        if (m_spotExponentUniform == 0) {
            UniformEntries::lazyGenEntry(&m_spotExponentUniform, getUniformName() + ".spotExponent");
        }

        _shader.setUniformf(UniformEntries::getEntry(m_directionUniform), direction);
        _shader.setUniformf(UniformEntries::getEntry(m_spotCosCutoffUniform), m_spotCosCutoff);
        _shader.setUniformf(UniformEntries::getEntry(m_spotExponentUniform), m_spotExponent);
    }
}

std::string SpotLight::getClassBlock() {
    if (s_classBlock.empty()) {
        s_classBlock = stringFromFile("shaders/spotLight.glsl", PathType::internal)+"\n";
    }
    return s_classBlock;
}

std::string SpotLight::getInstanceAssignBlock() {
    std::string block = Light::getInstanceAssignBlock();

    if (!m_dynamic) {
        block += ", " + glm::to_string(m_position);
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
