#include "pointLight.h"

#include "glm/gtx/string_cast.hpp"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "view/view.h"

namespace Tangram {

std::string PointLight::s_classBlock;
std::string PointLight::s_typeName = "PointLight";

PointLight::PointLight(const std::string& _name, bool _dynamic) :
    Light(_name, _dynamic),
    m_position(0.0),
    m_attenuation(0.0),
    m_innerRadius(0.0),
    m_outerRadius(0.0) {

    m_type = LightType::point;

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

void PointLight::setupProgram(const View& _view, ShaderProgram& _shader) {
    if (m_dynamic) {
        Light::setupProgram(_view, _shader);

        glm::vec4 position = m_position;

        if (m_origin == LightOrigin::world) {
            // For world origin, format is: [longitude, latitude, meters (default) or pixels w/px units]

            // Move light's world position into camera space
            glm::dvec2 camSpace = _view.getMapProjection().LonLatToMeters(glm::dvec2(m_position.x, m_position.y));
            position.x = camSpace.x - _view.getPosition().x;
            position.y = camSpace.y - _view.getPosition().y;
            position.z = position.z - _view.getPosition().z;

        } else if (m_origin == LightOrigin::ground) {
            // Leave light's xy in camera space, but z needs to be moved relative to ground plane
            position.z = position.z - _view.getPosition().z;
        }

        if (m_origin == LightOrigin::world || m_origin == LightOrigin::ground) {
            // Light position is a vector from the camera to the light in world space;
            // we can transform this vector into camera space the same way we would with normals
            position = _view.getViewMatrix() * position;
        }

        if (m_positionUniform == 0) {
            std::string positionUniformName = getUniformName() + ".position";
            if (UniformEntries::entryExistsForName(positionUniformName)) {
                UniformEntries::genEntry(&m_positionUniform, positionUniformName);
            }
        }

        _shader.setUniformf(UniformEntries::getEntry(m_positionUniform), position);

        if (m_attenuation != 0.0) {
            if (m_attenuationUniform == 0) {
                UniformEntries::lazyGenEntry(&m_attenuationUniform, getUniformName() + ".attenuation");
            }
            _shader.setUniformf(UniformEntries::getEntry(m_attenuationUniform), m_attenuation);
        }

        if (m_innerRadius!=0.0) {
            if (m_innerRadiusUniform == 0) {
                UniformEntries::lazyGenEntry(&m_innerRadiusUniform, getUniformName() + ".innerRadius");
            }
            _shader.setUniformf(UniformEntries::getEntry(m_innerRadiusUniform), m_innerRadius);
        }

        if (m_outerRadius!=0.0) {
            if (m_outerRadiusUniform == 0) {
                UniformEntries::lazyGenEntry(&m_outerRadiusUniform, getUniformName() + ".outerRadius");
            }
            _shader.setUniformf(UniformEntries::getEntry(m_outerRadiusUniform), m_outerRadius);
        }
    }
}

std::string PointLight::getClassBlock() {
    if (s_classBlock.empty()) {
        s_classBlock = stringFromFile("shaders/pointLight.glsl", PathType::internal)+"\n";
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

}
