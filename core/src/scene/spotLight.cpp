#include "spotLight.h"
#include "glm/gtx/string_cast.hpp"

std::string SpotLight::s_classBlock;

SpotLight::SpotLight(const std::string& _name, bool _dynamic):PointLight(_name,_dynamic),m_direction(1.0,0.0,0.0),m_spotExponent(0.0),m_spotCutoff(0.0),m_spotCosCutoff(0.0) {
    m_typeName = "SpotLight";
    m_type = LightType::SPOT;
}

SpotLight::~SpotLight() {

}

void SpotLight::setDirection(const glm::vec3 &_dir) {
    m_direction = _dir;
}

void SpotLight::setCutOff(float _cutoffAngle, float _exponent) {
    m_spotCutoff = _cutoffAngle;
    m_spotCosCutoff = cos(_cutoffAngle * 3.14159 / 180.0);
    m_spotExponent = _exponent;
}

void SpotLight::setupProgram(const std::shared_ptr<View>& _view, std::shared_ptr<ShaderProgram> _shader ) {
    if (m_dynamic) {
        PointLight::setupProgram(_view, _shader);

        m_direction_eye = m_direction;
        if (m_origin == LightOrigin::WORLD) {
            glm::mat3 normalMatrix = glm::mat3(_view->getViewMatrix()); // Transforms surface normals into camera space
            normalMatrix = glm::transpose(glm::inverse(normalMatrix));

            m_direction_eye = normalMatrix * m_direction;
        } else if (m_origin == LightOrigin::GROUND) {

            // TODO: this behaves weird
            //
            glm::mat3 normalMatrix = glm::mat3(_view->getViewMatrix()); // Transforms surface normals into camera space
            normalMatrix = glm::transpose(glm::inverse(normalMatrix));

            m_direction_eye = normalMatrix * m_direction;
        }

        _shader->setUniformf(getUniformName()+".direction", m_direction_eye);
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
