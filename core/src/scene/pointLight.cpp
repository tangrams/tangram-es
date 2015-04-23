#include "pointLight.h"
#include "glm/gtx/string_cast.hpp"

std::string PointLight::s_classBlock;

PointLight::PointLight(const std::string& _name, bool _dynamic):Light(_name,_dynamic),m_position(0.0),m_attenuation(0.0),m_innerRadius(0.0),m_outerRadius(0.0) {
    m_typeName = "PointLight";
    m_type = LightType::POINT;
}

PointLight::~PointLight() {

}

void PointLight::setPosition(const glm::vec3 &_pos) {
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 0.0;
}

void PointLight::setAttenuation(float _att){
    m_attenuation = _att;
}

void PointLight::setRadius(float _outer){
    m_innerRadius = 0.0;
    m_outerRadius = _outer;
}

void PointLight::setRadius(float _inner, float _outer){
    m_innerRadius = _inner;
    m_outerRadius = _outer;
}

void PointLight::setupProgram(std::shared_ptr<ShaderProgram> _shader) {
    if (m_dynamic) {
        Light::setupProgram(_shader);
        _shader->setUniformf(getUniformName()+".position", glm::vec4(m_position));

        if (m_attenuation!=0.0) {
            _shader->setUniformf(getUniformName()+".attenuation", m_attenuation);
        }

        if (m_innerRadius!=0.0) {
            _shader->setUniformf(getUniformName()+".innerRadius", m_innerRadius);
        }

        if (m_outerRadius!=0.0) {
            _shader->setUniformf(getUniformName()+".outerRadius", m_outerRadius);
        }
    }
}

std::string PointLight::getClassBlock() {
    if (s_classBlock.empty()) {
        s_classBlock = stringFromResource("pointLight.glsl")+"\n";
    }
    return s_classBlock;
}

std::string PointLight::getInstanceDefinesBlock() {
    std::string defines = "";
    
    if (m_attenuation!=0.0) {
        defines += "#define TANGRAM_POINTLIGHT_ATTENUATION_EXPONENT\n";
    }

    if (m_innerRadius!=0.0) {
        defines += "#define TANGRAM_POINTLIGHT_ATTENUATION_INNER_RADIUS\n";
    }

    if (m_outerRadius!=0.0) {
        defines += "#define TANGRAM_POINTLIGHT_ATTENUATION_OUTER_RADIUS\n";
    }
    return defines;
}

std::string PointLight::getInstanceAssignBlock() {
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
        block += ")";
    }
    return block;
}
