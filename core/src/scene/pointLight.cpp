#include "scene/pointLight.h"

#include "gl/shaderProgram.h"
#include "platform.h"
#include "pointLight_glsl.h"
#include "view/view.h"

#include "glm/gtx/string_cast.hpp"

namespace Tangram {

std::string PointLight::s_typeName = "PointLight";

PointLight::PointLight(const std::string& _name, bool _dynamic) :
    Light(_name, _dynamic),
    m_attenuation(0.0),
    m_innerRadius(0.0),
    m_outerRadius(0.0) {

    m_type = LightType::point;
}

PointLight::~PointLight() {}

void PointLight::setPosition(UnitVec<glm::vec3> pos) {
    m_position = pos;
}

void PointLight::setAttenuation(float _att) {
    m_attenuation = _att;
}

void PointLight::setRadius(float _outer) {
    m_innerRadius = 0.0;
    m_outerRadius = _outer;
}

void PointLight::setRadius(float _inner, float _outer) {
    m_innerRadius = _inner;
    m_outerRadius = _outer;
}

std::unique_ptr<LightUniforms> PointLight::getUniforms() {

    if (!m_dynamic) { return nullptr; }

    return std::make_unique<Uniforms>(getUniformName());
}

void PointLight::setupProgram(RenderState& rs, const View& _view, ShaderProgram& _shader,
                              LightUniforms& _uniforms) {
    Light::setupProgram(rs, _view, _shader, _uniforms);

    glm::vec4 position = glm::vec4(m_position.value, 0.0);

    if (m_origin == LightOrigin::world) {
        // For world origin, format is: [longitude, latitude, meters (default) or pixels w/px units]
        position[2] /= m_position.units[2] == Unit::pixel ? _view.pixelsPerMeter() : 1.0;

        // Move light's world position into camera space
        glm::dvec2 camSpace = _view.getMapProjection().LonLatToMeters(glm::dvec2(position.x, position.y));
        position.x = camSpace.x - (_view.getPosition().x + _view.getEye().x);
        position.y = camSpace.y - (_view.getPosition().y + _view.getEye().y);
        position.z = position.z - _view.getEye().z;

    } else if (m_origin == LightOrigin::ground || m_origin == LightOrigin::camera) {
        for (int i = 0; i < 3; ++i) {
            position[i] /= m_position.units[i] == Unit::pixel ? _view.pixelsPerMeter() : 1.0;
        }

        if (m_origin == LightOrigin::ground) {
            // Move light position relative to the eye position in world space
            position -= glm::vec4(_view.getEye(), 0.0);
        }
    }

    if (m_origin == LightOrigin::world || m_origin == LightOrigin::ground) {
        // Light position is a vector from the camera to the light in world space;
        // we can transform this vector into camera space the same way we would with normals
        position = _view.getViewMatrix() * position;
    }

    auto& u = static_cast<Uniforms&>(_uniforms);

    _shader.setUniformf(rs, u.position, position);

    if (m_attenuation != 0.0) {
        _shader.setUniformf(rs, u.attenuation, m_attenuation);
    }

    if (m_innerRadius != 0.0) {
        _shader.setUniformf(rs, u.innerRadius, m_innerRadius);
    }

    if (m_outerRadius != 0.0) {
        _shader.setUniformf(rs, u.outerRadius, m_outerRadius);
    }
}

std::string PointLight::getClassBlock() {
    return SHADER_SOURCE(pointLight_glsl);
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
        block += ")";
    }
    return block;
}

const std::string& PointLight::getTypeName() {

    return s_typeName;

}

}
