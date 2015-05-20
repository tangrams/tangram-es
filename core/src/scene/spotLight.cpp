#include "spotLight.h"
#include "glm/gtx/string_cast.hpp"

std::string SpotLight::s_classBlock;
std::string SpotLight::s_typeName = "SpotLight";

SpotLight::SpotLight(const std::string& _name, bool _dynamic) : 
    PointLight(_name, _dynamic),
    m_direction(1.0,0.0,0.0),
    m_spotExponent(0.0),
    m_spotCutoff(0.0),
    m_spotCosCutoff(0.0) {

    m_type = LightType::SPOT;
    
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

void SpotLight::setupProgram(const std::shared_ptr<View>& _view, std::shared_ptr<ShaderProgram> _shader ) {
    if (m_dynamic) {
        PointLight::setupProgram(_view, _shader);

        glm::vec3 direction = m_direction;
        if (m_origin == LightOrigin::WORLD) {
            direction = glm::normalize(_view->getNormalMatrix() * direction);
        }

        _shader->setUniformf(getUniformName()+".direction", direction);
        _shader->setUniformf(getUniformName()+".spotCosCutoff", m_spotCosCutoff);
        _shader->setUniformf(getUniformName()+".spotExponent", m_spotExponent);
    }
}

std::string SpotLight::getClassBlock() {
    if (s_classBlock.empty()) {
        s_classBlock = stringFromResource("spotLight.glsl")+"\n";
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
