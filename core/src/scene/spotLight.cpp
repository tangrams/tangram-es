#include "spotLight.h"
#include "util/stringsOp.h"

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
    m_spotCosCutoff = cos(_cutoffAngle * 3.14159 / 180.0);//cos(_cutoff);
    m_spotExponent = _exponent;
}

void SpotLight::setupProgram( std::shared_ptr<ShaderProgram> _shader ) {
    if (m_dynamic) {
        PointLight::setupProgram(_shader);
        _shader->setUniformf(getUniformName()+".direction", m_direction);
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

std::string SpotLight::getInstanceDefinesBlock() {
    std::string defines = "";

    if (m_constantAttenuation!=0.0) {
        defines += "#define TANGRAM_SPOTLIGHT_CONSTANT_ATTENUATION\n";
    }

    if (m_linearAttenuation!=0.0) {
        defines += "#define TANGRAM_SPOTLIGHT_LINEAR_ATTENUATION\n";
    }

    if (m_quadraticAttenuation!=0.0) {
        defines += "#define TANGRAM_SPOTLIGHT_QUADRATIC_ATTENUATION\n";
    }
    return defines;
}

std::string SpotLight::getInstanceAssignBlock() {
    std::string block = Light::getInstanceAssignBlock();

    if (!m_dynamic) {

        block += ", " + getString(m_position);
        block += ", " + getString(m_direction);

        block += ", " + getString(m_spotCosCutoff);
        block += ", " + getString(m_spotExponent,8);

        if (m_constantAttenuation!=0.0) {
            block += ", " + getString(m_constantAttenuation);
        }
        if (m_linearAttenuation!=0.0) {
            block += ", " + getString(m_linearAttenuation);
        }
        if (m_quadraticAttenuation!=0.0) {
            block += ", " + getString(m_quadraticAttenuation);
        }

        block += ")";
    }
    return block;
}
