#include "spotLight.h"


#include "gl/shaderProgram.h"
#include "style/material.h"
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

std::unique_ptr<LightUniforms> SpotLight::getUniforms(ShaderProgram& _shader) {

    if (m_dynamic) {
        return std::make_unique<Uniforms>(_shader, getUniformName());
    }
    return nullptr;
}

void SpotLight::setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms ) {
    PointLight::setupProgram(rs, _view, _uniforms);

    glm::vec3 direction = m_direction;
    if (m_origin == LightOrigin::world) {
        direction = glm::normalize(_view.getNormalMatrix() * direction);
    }

    auto& u = static_cast<Uniforms&>(_uniforms);
    u.shader.setUniformf(rs, u.direction, direction);
    u.shader.setUniformf(rs, u.spotCosCutoff, m_spotCosCutoff);
    u.shader.setUniformf(rs, u.spotExponent, m_spotExponent);
}

void SpotLight::buildClassBlock(Material& _material, ShaderSource& out) {

    out << "struct SpotLight {";
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
    out << "    vec3 direction;";
    out << "    float spotCosCutoff;";
    out << "    float spotExponent;";
    out << "};";

    out << "void calculateLight(in SpotLight _light, in vec3 _eyeToPoint, in vec3 _normal) {";
    out << "    float dist = length(_light.position.xyz - _eyeToPoint);";
    // Compute vector from surface to light position
    out << "    vec3 VP = (_light.position.xyz - _eyeToPoint) / dist;";
    // normal . light direction
    out << "    float nDotVP = clamp(dot(_normal, VP), 0.0, 1.0);";
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
    // spotlight attenuation factor
    out << "    float spotAttenuation = 0.0;";
    // See if point on surface is inside cone of illumination
    out << "    float spotDot = clamp(dot(-VP, normalize(_light.direction)), 0.0, 1.0);";
    out << "    if (spotDot >= _light.spotCosCutoff) {";
    out << "        spotAttenuation = pow(spotDot, _light.spotExponent);";
    out << "    }";
    out << "    light_accumulator_ambient += _light.ambient * attenuation * spotAttenuation;";
    if (_material.hasDiffuse()) {
        out << "    light_accumulator_diffuse += _light.diffuse * nDotVP * attenuation * spotAttenuation;";
    }
    if (_material.hasSpecular()) {
        // Power factor for shiny speculars
        out << "    float pf = 0.0;";
        out << "    if (nDotVP > 0.0) {";
        out << "        vec3 reflectVector = reflect(-VP, _normal);";
        out << "        float eyeDotR = max(dot(-normalize(_eyeToPoint), reflectVector), 0.0);";
        out << "        pf = pow(eyeDotR, material.shininess);";
        out << "    }";
        out << "    light_accumulator_specular += _light.specular * pf * attenuation * spotAttenuation;";
    }
    out << "}";
}

void SpotLight::buildInstanceAssignBlock(ShaderSource& out) {
    Light::buildInstanceAssignBlock(out);

    if (!m_dynamic) {
        out += ", " + glm::to_string(m_position.value);
        if (m_attenuation!=0.0) {
            out += ", " + std::to_string(m_attenuation);
        }
        if (m_innerRadius!=0.0) {
            out += ", " + std::to_string(m_innerRadius);
        }
        if (m_outerRadius!=0.0) {
            out += ", " + std::to_string(m_outerRadius);
        }

        out += ", " + glm::to_string(m_direction);
        out += ", " + std::to_string(m_spotCosCutoff);
        out += ", " + std::to_string(m_spotExponent);
    }
}

const std::string& SpotLight::getTypeName() {

    return s_typeName;

}

}
