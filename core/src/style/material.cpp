#include "material.h"

#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"

namespace Tangram {

Material::Material() {
}

void Material::setEmission(glm::vec4 _emission){
    m_emission = _emission;
    m_emission_texture.tex.reset();
    setEmissionEnabled(true);
}

void Material::setEmission(MaterialTexture _emissionTexture){
    m_emission_texture = _emissionTexture;
    m_emission = glm::vec4(m_emission_texture.amount, 1.f);
    setEmissionEnabled(bool(m_emission_texture.tex));
}

void Material::setAmbient(glm::vec4 _ambient){
    m_ambient = _ambient;
    m_ambient_texture.tex.reset();
    setAmbientEnabled(true);
}

void Material::setAmbient(MaterialTexture _ambientTexture){
    m_ambient_texture = _ambientTexture;
    m_ambient = glm::vec4(m_ambient_texture.amount, 1.f);
    setAmbientEnabled(bool(m_ambient_texture.tex));
}

void Material::setDiffuse(glm::vec4 _diffuse){
    m_diffuse = _diffuse;
    m_diffuse_texture.tex.reset();
    setDiffuseEnabled(true);
}

void Material::setDiffuse(MaterialTexture _diffuseTexture){
    m_diffuse_texture = _diffuseTexture;
    m_diffuse = glm::vec4(m_diffuse_texture.amount, 1.f);
    setDiffuseEnabled(bool(m_diffuse_texture.tex));
}

void Material::setSpecular(glm::vec4 _specular){
    m_specular = _specular;
    m_specular_texture.tex.reset();
    setSpecularEnabled(true);
}

void Material::setSpecular(MaterialTexture _specularTexture){
    m_specular_texture = _specularTexture;
    m_specular = glm::vec4(m_specular_texture.amount, 1.f);
    setSpecularEnabled(bool(m_specular_texture.tex));
}

void Material::setShininess(float _shiny) {
    m_shininess = _shiny;
    setSpecularEnabled(true);
}

void Material::setEmissionEnabled(bool _enable) { m_bEmission = _enable; }
void Material::setAmbientEnabled(bool _enable) { m_bAmbient = _enable; }
void Material::setDiffuseEnabled(bool _enable) { m_bDiffuse = _enable; }
void Material::setSpecularEnabled(bool _enable) { m_bSpecular = _enable; }

void Material::setNormal(MaterialTexture _normalTexture){
    m_normal_texture = _normalTexture;
    if (m_normal_texture.mapping == MappingType::spheremap){
        m_normal_texture.mapping = MappingType::planar;
    }
}

std::string mappingTypeToString(MappingType type) {
    switch(type) {
        case MappingType::uv:        return "UV";
        case MappingType::planar:    return "PLANAR";
        case MappingType::triplanar: return "TRIPLANAR";
        case MappingType::spheremap: return "SPHEREMAP";
        default:                     return "";
    }
}

void Material::buildMaterialBlock(ShaderSource& out) {

    out << "#define TANGRAM_SKEW vec2(0.0)";

    out << "struct Material {";
    if (m_bEmission) {
        out << "    vec4 emission;";
        if (m_emission_texture.tex) {
            out << "    vec3 emissionScale;";
        }
    }
    if (m_bAmbient) {
        out << "    vec4 ambient;";
        if (m_ambient_texture.tex) {
            out << "    vec3 ambientScale;";
        }
    }
    if (m_bDiffuse) {
        out << "    vec4 diffuse;";
        if (m_diffuse_texture.tex) {
            out << "    vec3 diffuseScale;";
        }
    }
    if (m_bSpecular) {
        out << "    vec4 specular;";
        out << "    float shininess;";
        if (m_specular_texture.tex) {
            out << "    vec3 specularScale;";
        }
    }
    if (m_normal_texture.tex) {
        out << "    vec3 normalScale;";
        out << "    vec3 normalAmount;";
    }
    out << "};";

    // Note: uniform is copied to a global instance to allow modification
    out << "uniform Material u_material;";
    out << "Material material;";
}

void Material::buildMaterialFragmentBlock(ShaderSource& out) {

    if (m_emission_texture.tex) {
        out << "uniform sampler2D u_material_emission_texture;";
    }
    if (m_ambient_texture.tex) {
        out << "uniform sampler2D u_material_ambient_texture;";
    }
    if (m_diffuse_texture.tex) {
        out << "uniform sampler2D u_material_diffuse_texture;";
    }
    if (m_specular_texture.tex) {
        out << "uniform sampler2D u_material_specular_texture;";
    }
    if (m_normal_texture.tex) {
        out << "uniform sampler2D u_material_normal_texture;";
    }

    if (m_emission_texture.mapping == MappingType::spheremap ||
        m_ambient_texture.mapping == MappingType::spheremap ||
        m_diffuse_texture.mapping == MappingType::spheremap ||
        m_normal_texture.mapping == MappingType::spheremap) {

        out << "vec4 getSphereMap (in sampler2D _tex, in vec3 _eyeToPoint, in vec3 _normal, in vec2 _skew) {"
            << "    vec3 eye = normalize(_eyeToPoint);"
            << "    eye.xy -= _skew;"
            << "    eye = normalize(eye);"
            << "    vec3 r = reflect(eye, _normal);"
            << "    r.z += 1.0;"
            << "    float m = 2. * length(r);"
            << "    vec2 uv = r.xy / m + .5;"
            << "    return texture2D(_tex, uv);"
            << "}";
    }

    if (m_emission_texture.mapping == MappingType::triplanar ||
        m_ambient_texture.mapping == MappingType::triplanar ||
        m_diffuse_texture.mapping == MappingType::triplanar ||
        m_normal_texture.mapping == MappingType::triplanar) {

        out << "vec3 getTriPlanarBlend (in vec3 _normal) {"
            << "    vec3 blending = abs(_normal);"
            << "    blending = normalize(max(blending, 0.00001));"
            << "    float b = (blending.x + blending.y + blending.z);"
            << "    return blending / b;"
            << "}";

        out << "vec4 getTriPlanar (in sampler2D _tex, in vec3 _pos, in vec3 _normal, in vec3 _scale) {"
            << "    vec3 blending = getTriPlanarBlend(_normal);"
            << "    vec4 xaxis = texture2D(_tex, fract(_pos.yz * _scale.x));"
            << "    vec4 yaxis = texture2D(_tex, fract(_pos.xz * _scale.y));"
            << "    vec4 zaxis = texture2D(_tex, fract(_pos.xy * _scale.z));"
            << "    return  xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;"
            << "}";
    }

    if (m_emission_texture.mapping == MappingType::planar ||
        m_ambient_texture.mapping == MappingType::planar ||
        m_diffuse_texture.mapping == MappingType::planar ||
        m_normal_texture.mapping == MappingType::planar) {

        out << "vec4 getPlanar (in sampler2D _tex, in vec3 _pos, in vec2 _scale) {"
            << "    return texture2D( _tex, fract(_pos.xy * _scale.x) );"
            << "}";
    }

    if (m_normal_texture.tex) {
        out << "void calculateNormal (inout vec3 _normal) {";
        switch(m_normal_texture.mapping) {
        case MappingType::uv:
            out << "    _normal += texture2D(u_material_normal_texture, fract(v_texcoord*material.normalScale.xy)).rgb*2.0-1.0;";
            break;
        case MappingType::planar:
            out << "    _normal += getPlanar(u_material_normal_texture, v_world_position.xyz, material.normalScale.xy).rgb*2.0-1.0;";
            break;
        case MappingType::triplanar:
            out << "    _normal += getTriPlanar(u_material_normal_texture, v_world_position.xyz, _normal, material.normalScale).rgb*2.0-1.0;";
            break;
        default: break;
        }
        out << "    _normal = normalize(_normal);";
        out << "}";
    }

    out << "void calculateMaterial (in vec3 _eyeToPoint, inout vec3 _normal) {";
    if (m_emission_texture.tex) {
        switch(m_emission_texture.mapping) {
        case MappingType::uv:
            out << "    material.emission *= texture2D(u_material_emission_texture, v_texcoord);";
            break;
        case MappingType::planar:
            out << "    material.emission *= getPlanar(u_material_emission_texture, v_world_position.xyz, material.emissionScale.xy);";
            break;
        case MappingType::triplanar:
            out << "    material.emission *= getTriPlanar(u_material_emission_texture, v_world_position.xyz, _normal, material.emissionScale);";
            break;
        case MappingType::spheremap:
            out << "    material.emission *= getSphereMap(u_material_emission_texture, _eyeToPoint, _normal, TANGRAM_SKEW);";
            break;
        default: break;
        }
    }
    if (m_ambient_texture.tex) {
        switch(m_ambient_texture.mapping) {
        case MappingType::uv:
            out << "    material.ambient *= texture2D(u_material_ambient_texture, v_texcoord);";
            break;
        case MappingType::planar:
            out << "    material.ambient *= getPlanar(u_material_ambient_texture, v_world_position.xyz, material.ambientScale.xy);";
            break;
        case MappingType::triplanar:
            out << "    material.ambient *= getTriPlanar(u_material_ambient_texture, v_world_position.xyz, _normal, material.ambientScale);";
            break;
        case MappingType::spheremap:
            out << "    material.ambient *= getSphereMap(u_material_ambient_texture, _eyeToPoint, _normal, TANGRAM_SKEW);";
            break;
        default: break;
        }
    }
    if (m_diffuse_texture.tex) {
        switch(m_diffuse_texture.mapping) {
        case MappingType::uv:
            out << "    material.diffuse *= texture2D(u_material_diffuse_texture, v_texcoord);";
            break;
        case MappingType::planar:
            out << "    material.diffuse *= getPlanar(u_material_diffuse_texture, v_world_position.xyz, material.diffuseScale.xy);";
            break;
        case MappingType::triplanar:
            out << "    material.diffuse *= getTriPlanar(u_material_diffuse_texture, v_world_position.xyz, _normal, material.diffuseScale);";
            break;
        case MappingType::spheremap:
            out << "    material.diffuse *= getSphereMap(u_material_diffuse_texture, _eyeToPoint, _normal, TANGRAM_SKEW);";
            break;
        default: break;
        }
    }
    if (m_specular_texture.tex) {
        switch(m_specular_texture.mapping) {
        case MappingType::uv:
            out << "    material.specular *= texture2D(u_material_specular_texture, v_texcoord);";
            break;
        case MappingType::planar:
            out << "    material.specular *= getPlanar(u_material_specular_texture, v_world_position.xyz, material.specularScale.xy);";
            break;
        case MappingType::triplanar:
            out << "    material.specular *= getTriPlanar(u_material_specular_texture, v_world_position.xyz, _normal, material.specularScale);";
            break;
        case MappingType::spheremap:
            out << "    material.specular *= getSphereMap(u_material_specular_texture, _eyeToPoint, _normal, TANGRAM_SKEW);";
            break;
        default: break;
        }
    }
    out << "}";
}

std::unique_ptr<MaterialUniforms> Material::getUniforms(ShaderProgram& _shader ) {

    if (m_bEmission || m_bAmbient || m_bDiffuse || m_bSpecular || m_normal_texture.tex) {
        return std::make_unique<MaterialUniforms>(_shader);
    }
    return nullptr;
}

void Material::setupProgram(RenderState& rs, MaterialUniforms& _uniforms) {

    auto& u = _uniforms;

    if (m_bEmission) {
        u.shader.setUniformf(rs, u.emission, m_emission);

        if (m_emission_texture.tex) {
            m_emission_texture.tex->update(rs, rs.nextAvailableTextureUnit());
            m_emission_texture.tex->bind(rs, rs.currentTextureUnit());
            u.shader.setUniformi(rs, u.emissionTexture, rs.currentTextureUnit());
            u.shader.setUniformf(rs, u.emissionScale, m_emission_texture.scale);
        }
    }

    if (m_bAmbient) {
        u.shader.setUniformf(rs, u.ambient, m_ambient);

        if (m_ambient_texture.tex) {
            m_ambient_texture.tex->update(rs, rs.nextAvailableTextureUnit());
            m_ambient_texture.tex->bind(rs, rs.currentTextureUnit());
            u.shader.setUniformi(rs, u.ambientTexture, rs.currentTextureUnit());
            u.shader.setUniformf(rs, u.ambientScale, m_ambient_texture.scale);
        }
    }

    if (m_bDiffuse) {
        u.shader.setUniformf(rs, u.diffuse, m_diffuse);

        if (m_diffuse_texture.tex) {
            m_diffuse_texture.tex->update(rs, rs.nextAvailableTextureUnit());
            m_diffuse_texture.tex->bind(rs, rs.currentTextureUnit());
            u.shader.setUniformi(rs, u.diffuseTexture, rs.currentTextureUnit());
            u.shader.setUniformf(rs, u.diffuseScale, m_diffuse_texture.scale);
        }
    }

    if (m_bSpecular) {
        u.shader.setUniformf(rs, u.specular, m_specular);
        u.shader.setUniformf(rs, u.shininess, m_shininess);

        if (m_specular_texture.tex) {
            m_specular_texture.tex->update(rs, rs.nextAvailableTextureUnit());
            m_specular_texture.tex->bind(rs, rs.currentTextureUnit());
            u.shader.setUniformi(rs, u.specularTexture, rs.currentTextureUnit());
            u.shader.setUniformf(rs, u.specularScale, m_specular_texture.scale);
        }
    }

    if (m_normal_texture.tex) {
        m_normal_texture.tex->update(rs, rs.nextAvailableTextureUnit());
        m_normal_texture.tex->bind(rs, rs.currentTextureUnit());
        u.shader.setUniformi(rs, u.normalTexture, rs.currentTextureUnit());
        u.shader.setUniformf(rs, u.normalScale, m_normal_texture.scale);
        u.shader.setUniformf(rs, u.normalAmount, m_normal_texture.amount);
    }
}

}
