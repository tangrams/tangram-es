#include "pointLight.h"
#include "glm/gtx/string_cast.hpp"

std::string PointLight::s_classBlock;
std::string PointLight::s_typeName = "PointLight";

PointLight::PointLight(const std::string& _name, bool _dynamic) : 
    Light(_name, _dynamic),
    m_position(0.0),
    m_attenuation(0.0),
    m_innerRadius(0.0),
    m_outerRadius(0.0) {

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

void PointLight::setupProgram(const std::shared_ptr<View>& _view, std::shared_ptr<ShaderProgram> _shader) {
    if (m_dynamic) {
        Light::setupProgram(_view, _shader);

        glm::vec4 position = m_position;

        if (m_origin == LightOrigin::WORLD) {
            // For world origin, format is: [longitude, latitude, meters (default) or pixels w/px units]

            // Move light's world position into camera space
            glm::dvec2 camSpace = _view->getMapProjection().LonLatToMeters(glm::dvec2(m_position.x, m_position.y));
            position.x = camSpace.x - _view->getPosition().x;
            position.y = camSpace.y - _view->getPosition().y;
            position.z = position.z - _view->getPosition().z;

        } else if (m_origin == LightOrigin::GROUND) {
            // Leave light's xy in camera space, but z needs to be moved relative to ground plane
            position.z = position.z - _view->getPosition().z;
        }
        
        if (m_origin == LightOrigin::WORLD || m_origin == LightOrigin::GROUND) {
            // Light position is a vector from the camera to the light in world space;
            // we can transform this vector into camera space the same way we would with normals
            position = _view->getViewMatrix() * position;
        }

        _shader->setUniformf(getUniformName()+".position", position);

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

const std::string& PointLight::getTypeName() {

    return s_typeName;

}
