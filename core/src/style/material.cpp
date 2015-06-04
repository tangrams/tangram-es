#include "material.h"

#include "shaderProgram.h"
#include "texture.h"

Material::Material() {
}

void Material::setEmission(const glm::vec4 _emission){
    m_emission = _emission;
    m_emission_texture.tex.reset();
    setEmissionEnabled(true);
}

void Material::setEmission(MaterialTexture _emissionTexture){
    m_emission_texture = _emissionTexture;
    m_emission = glm::vec4(m_emission_texture.amount, 1.f);
    setEmissionEnabled((bool)m_emission_texture.tex);
}

void Material::setAmbient(const glm::vec4 _ambient){
    m_ambient = _ambient;
    m_ambient_texture.tex.reset();
    setAmbientEnabled(true);
}

void Material::setAmbient(MaterialTexture _ambientTexture){
    m_ambient_texture = _ambientTexture;
    m_ambient = glm::vec4(m_ambient_texture.amount, 1.f);
    setAmbientEnabled((bool)m_ambient_texture.tex);
}

void Material::setDiffuse(const glm::vec4 _diffuse){
    m_diffuse = _diffuse;
    m_diffuse_texture.tex.reset();
    setDiffuseEnabled(true);
}

void Material::setDiffuse(MaterialTexture _diffuseTexture){
    m_diffuse_texture = _diffuseTexture;
    m_diffuse = glm::vec4(m_diffuse_texture.amount, 1.f);
    setDiffuseEnabled((bool)m_diffuse_texture.tex);
}

void Material::setSpecular(const glm::vec4 _specular){
    m_specular = _specular;
    m_specular_texture.tex.reset();
    setSpecularEnabled(true);
}

void Material::setSpecular(MaterialTexture _specularTexture){
    m_specular_texture = _specularTexture;
    m_specular = glm::vec4(m_specular_texture.amount, 1.f);
    setSpecularEnabled((bool)m_specular_texture.tex);
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
    }
}

std::string Material::getDefinesBlock(){
    std::string defines = "";
    
    bool mappings[4] = { false };

    if (m_bEmission) {
        defines += "#define TANGRAM_MATERIAL_EMISSION\n";
        if (m_emission_texture.tex) {
            defines += "#define TANGRAM_MATERIAL_EMISSION_TEXTURE\n";
            defines += "#define TANGRAM_MATERIAL_EMISSION_TEXTURE_" + mappingTypeToString(m_emission_texture.mapping) + "\n";
            mappings[(int)m_emission_texture.mapping] = true;
        }
    }

    if (m_bAmbient) {
        defines += "#define TANGRAM_MATERIAL_AMBIENT\n";
        if (m_ambient_texture.tex) {
            defines += "#define TANGRAM_MATERIAL_AMBIENT_TEXTURE\n";
            defines += "#define TANGRAM_MATERIAL_AMBIENT_TEXTURE_" + mappingTypeToString(m_ambient_texture.mapping) + "\n";
            mappings[(int)m_ambient_texture.mapping] = true;
        }
    }

    if (m_bDiffuse) {
        defines += "#define TANGRAM_MATERIAL_DIFFUSE\n";
        if (m_diffuse_texture.tex) {
            defines += "#define TANGRAM_MATERIAL_DIFFUSE_TEXTURE\n";
            defines += "#define TANGRAM_MATERIAL_DIFFUSE_TEXTURE_" + mappingTypeToString(m_diffuse_texture.mapping) + "\n";
            mappings[(int)m_diffuse_texture.mapping] = true;
        }
    }

    if (m_bSpecular) {
        defines += "#define TANGRAM_MATERIAL_SPECULAR\n";
        if (m_specular_texture.tex) {
            defines += "#define TANGRAM_MATERIAL_SPECULAR_TEXTURE\n";
            defines += "#define TANGRAM_MATERIAL_SPECULAR_TEXTURE_" + mappingTypeToString(m_specular_texture.mapping) + "\n";
            mappings[(int)m_specular_texture.mapping] = true;
        }
    }

    if (m_normal_texture.tex){
        defines += "#define TANGRAM_MATERIAL_NORMAL_TEXTURE\n";
        defines += "#define TANGRAM_MATERIAL_NORMAL_TEXTURE_" + mappingTypeToString(m_normal_texture.mapping) + "\n";
        mappings[(int)m_specular_texture.mapping] = true;
    }
    
    for (int i = 0; i < 4; i++) {
        if (mappings[i]) {
            defines += "#define TANGRAM_MATERIAL_TEXTURE_" + mappingTypeToString((MappingType)i) + "\n";
        }
    }

    return defines;
}

std::string Material::getClassBlock() {
    return stringFromResource("material.glsl") + "\n";
}

void Material::injectOnProgram(std::shared_ptr<ShaderProgram> _shader ) {
    _shader->addSourceBlock("defines", getDefinesBlock(), false);
    _shader->addSourceBlock("material", getClassBlock(), false);
}

void Material::setupProgram(std::shared_ptr<ShaderProgram> _shader) {

    if (m_bEmission) {
        _shader->setUniformf("u_material.emission", m_emission);

        if (m_emission_texture.tex) {
            m_emission_texture.tex->update(1);
            m_emission_texture.tex->bind(1);
            _shader->setUniformi("u_material_emission_texture", 1);
            _shader->setUniformf("u_material.emissionScale", m_emission_texture.scale);
        }
    }

    if (m_bAmbient) {
        _shader->setUniformf("u_material.ambient", m_ambient);

        if (m_ambient_texture.tex) {
            m_ambient_texture.tex->update(2);
            m_ambient_texture.tex->bind(2);
            _shader->setUniformi("u_material_ambient_texture", 2);
            _shader->setUniformf("u_material.ambientScale", m_ambient_texture.scale);
        }
    }

    if (m_bDiffuse) {
        _shader->setUniformf("u_material.diffuse", m_diffuse);

        if (m_diffuse_texture.tex) {
            m_diffuse_texture.tex->update(3);
            m_diffuse_texture.tex->bind(3);
            _shader->setUniformi("u_material_diffuse_texture", 3);
            _shader->setUniformf("u_material.diffuseScale", m_diffuse_texture.scale);
        }
    }

    if (m_bSpecular) {
        _shader->setUniformf("u_material.specular", m_specular);
        _shader->setUniformf("u_material.shininess", m_shininess);

        if (m_diffuse_texture.tex) {
            m_diffuse_texture.tex->update(4);
            m_diffuse_texture.tex->bind(4);
            _shader->setUniformi("u_material_specular_texture", 4);
            _shader->setUniformf("u_material.specularScale", m_specular_texture.scale);
        }
    }

    if (m_normal_texture.tex) {
        m_normal_texture.tex->update(5);
        m_normal_texture.tex->bind(5);
        _shader->setUniformi("u_material_normal_texture", 5);
        _shader->setUniformf("u_material.normalScale", m_normal_texture.scale);
        _shader->setUniformf("u_material.normalAmount", m_normal_texture.amount);
    }
}
