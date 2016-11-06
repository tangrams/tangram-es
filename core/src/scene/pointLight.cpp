#include "pointLight.h"

#include "gl/shaderProgram.h"
#include "style/material.h"
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

std::unique_ptr<LightUniforms> PointLight::getUniforms(ShaderProgram& _shader) {

    if (m_dynamic) {
        return std::make_unique<Uniforms>(_shader, getUniformName());
    }
    return nullptr;
}

void PointLight::setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms) {
    Light::setupProgram(rs, _view, _uniforms);

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

    u.shader.setUniformf(rs, u.position, position);

    if (m_attenuation != 0.0) {
        u.shader.setUniformf(rs, u.attenuation, m_attenuation);
    }

    if (m_innerRadius != 0.0) {
        u.shader.setUniformf(rs, u.innerRadius, m_innerRadius);
    }

    if (m_outerRadius != 0.0) {
        u.shader.setUniformf(rs, u.outerRadius, m_outerRadius);
    }
}

void PointLight::buildClassBlock(Material& _material, ShaderSource& out) {

    out << "struct PointLight {";
    out << "    vec4 ambient;";
    out << "    vec4 diffuse;";
    out << "    vec4 specular;";
    out << "    vec4 position;";
    if (m_attenuation != 0.0) {
        out << "    float attenuationExponent;";
    }
    if (m_innerRadius != 0.0) {
        out << "    float innerRadius;";
    }
    if (m_outerRadius != 0.0) {
        out << "    float outerRadius;";
    }
    out << "};";

    out << "void calculateLight(in PointLight _light, in vec3 _eyeToPoint, in vec3 _normal) {";
    out << "    float dist = length(_light.position.xyz - _eyeToPoint);";
    // Compute vector from surface to light position
    out << "    vec3 VP = (_light.position.xyz - _eyeToPoint) / dist;";
    // Normalize the vector from surface to light position
    out << "    float nDotVP = clamp(dot(VP, _normal), 0.0, 1.0);";
    // Attenuation defaults
    out << "    float attenuation = 1.0;";
    if (m_attenuation != 0.0) {
        out << "    float Rin = 1.0;";
        out << "    float e = _light.attenuationExponent;";
        if (m_innerRadius != 0.0) {
            out << "    Rin = _light.innerRadius;";
        }
        if (m_outerRadius != 0.0) {
            out << "    float Rdiff = _light.outerRadius-Rin;";
            out << "    float d = clamp(max(0.0,dist-Rin)/Rdiff, 0.0, 1.0);";
            out << "    attenuation = 1.0-(pow(d,e));";
        } else {
            // If no outer is provide behaves like:
            // https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
            out << "    float d = max(0.0,dist-Rin)/Rin+1.0;";
            out << "    attenuation = clamp(1.0/(pow(d,e)), 0.0, 1.0);";
        }
    } else {
        out << "    float Rin = 0.0;";
        if (m_innerRadius != 0.0) {
            out << "    Rin = _light.innerRadius;";
            if (m_outerRadius != 0.0) {
                out << "    float Rdiff = _light.outerRadius-Rin;";
                out << "    float d = clamp(max(0.0,dist-Rin)/Rdiff, 0.0, 1.0);";
                out << "    attenuation = 1.0-d*d;";
            } else {
                // If no outer is provide behaves like:
                // https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
                out << "    float d = max(0.0,dist-Rin)/Rin+1.0;";
                out << "    attenuation = clamp(1.0/d, 0.0, 1.0);";
            }
        } else {
            if (m_outerRadius != 0.0) {
                out << "    float d = clamp(dist/_light.outerRadius, 0.0, 1.0);";
                out << "    attenuation = 1.0-d*d;";
            } else {
                out << "    attenuation = 1.0;";
            }
        }
    }
    // Computer accumulators
    out << "    light_accumulator_ambient += _light.ambient * attenuation;";

    if (_material.hasDiffuse()) {
        out << "    light_accumulator_diffuse += _light.diffuse * nDotVP * attenuation;";
    }
    if (_material.hasSpecular()) {
        // power factor for shiny speculars
        out << "    float pf = 0.0;";
        out << "    if (nDotVP > 0.0) {";
        out << "        vec3 reflectVector = reflect(-VP, _normal);";
        out << "        float eyeDotR = max(0.0, dot(-normalize(_eyeToPoint), reflectVector));";
        out << "        pf = pow(eyeDotR, material.shininess);";
        out << "    }";
        out << "    light_accumulator_specular += _light.specular * pf * attenuation;";
    }
    out << "}";
}

void PointLight::buildInstanceAssignBlock(ShaderSource& out) {
    Light::buildInstanceAssignBlock(out);
    if (!m_dynamic) {
        out += ", " + glm::to_string(m_position.value);
        if (m_attenuation != 0.0) {
            out += ", " + std::to_string(m_attenuation);
        }
        if (m_innerRadius != 0.0) {
            out += ", " + std::to_string(m_innerRadius);
        }
        if (m_outerRadius != 0.0) {
            out += ", " + std::to_string(m_outerRadius);
        }
    }
}

const std::string& PointLight::getTypeName() {

    return s_typeName;

}

}
