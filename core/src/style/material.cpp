#include "material.h"

#include "shaderProgram.h"
#include "texture.h"

Material::Material() {
}

void Material::setEmission(const glm::vec4 _emission){
    m_emission = _emission;
    m_emission_texture.reset();
    setEmissionEnabled(true);
}

void Material::setEmission(const std::string &_file, MappingType _type, glm::vec3 _scale, glm::vec4 _amount){
    std::shared_ptr<Texture> texture(new Texture(_file));
    setEmission(texture, _type, _scale, _amount);
}

void Material::setEmission(std::shared_ptr<Texture> _texture, MappingType _type, glm::vec3 _scale, glm::vec4 _amount){
    m_emission_texture = _texture;
    m_emission_texture_mapping = _type;
    m_emission_texture_scale = _scale;
    m_emission = _amount;
    setEmissionEnabled((bool)_texture);
}

void Material::setAmbient(const glm::vec4 _ambient){
    m_ambient = _ambient;
    m_ambient_texture.reset();
    setAmbientEnabled(true);
}

void Material::setAmbient(const std::string &_file, MappingType _type, glm::vec3 _scale, glm::vec4 _amount){
    std::shared_ptr<Texture> texture(new Texture(_file));
    setAmbient(texture, _type, _scale, _amount);
}

void Material::setAmbient(std::shared_ptr<Texture> _texture, MappingType _type, glm::vec3 _scale, glm::vec4 _amount){
    m_ambient_texture = _texture;
    m_ambient_texture_mapping = _type;
    m_ambient_texture_scale = _scale;
    m_ambient = _amount;
    setAmbientEnabled((bool)_texture);
}

void Material::setDiffuse(const glm::vec4 _diffuse){
    m_diffuse = _diffuse;
    m_diffuse_texture.reset();
    setDiffuseEnabled(true);
}

void Material::setDiffuse(const std::string &_file, MappingType _type, glm::vec3 _scale, glm::vec4 _amount){
    std::shared_ptr<Texture> texture(new Texture(_file));
    setDiffuse(texture, _type, _scale, _amount);
}

void Material::setDiffuse(std::shared_ptr<Texture> _texture, MappingType _type, glm::vec3 _scale, glm::vec4 _amount){
    m_diffuse_texture = _texture;
    m_diffuse_texture_mapping = _type;
    m_diffuse_texture_scale = _scale;
    m_diffuse = _amount;
    setDiffuseEnabled((bool)_texture);
}

void Material::setSpecular(const glm::vec4 _specular, float _shininess){
    m_specular = _specular;
    m_shininess = _shininess;
    m_specular_texture.reset();
    setSpecularEnabled(true);
}

void Material::setSpecular(const std::string &_file, MappingType _type, glm::vec3 _scale, glm::vec4 _amount){
    std::shared_ptr<Texture> texture(new Texture(_file));
    setSpecular(texture, _type, _scale, _amount);
}

void Material::setSpecular(std::shared_ptr<Texture> _texture, MappingType _type, glm::vec3 _scale, glm::vec4 _amount){
    m_specular_texture = _texture;
    m_specular_texture_mapping = _type;
    m_specular_texture_scale = _scale;
    m_specular = _amount;
    setSpecularEnabled((bool)_texture);
}

void Material::setEmissionEnabled(bool _enable) { m_bEmission = _enable; }
void Material::setAmbientEnabled(bool _enable) { m_bAmbient = _enable; }
void Material::setDiffuseEnabled(bool _enable) { m_bDiffuse = _enable; }
void Material::setSpecularEnabled(bool _enable) { m_bSpecular = _enable; }

void Material::setNormal(const std::string &_file, MappingType _type, glm::vec3 _scale, float _amount){
    std::shared_ptr<Texture> texture(new Texture(_file));
    setNormal(texture, _type, _scale, _amount);
}

void Material::setNormal(std::shared_ptr<Texture> _texture, MappingType _type, glm::vec3 _scale, float _amount){
    m_normal_texture = _texture;
    if (_type == MappingType::SPHEREMAP){
        // TODO 
        //      - Print error: no SPHEREMAP on normal 
        _type = MappingType::PLANAR;
    }
    m_normal_texture_mapping = _type;
    m_normal_texture_scale = _scale;
    m_normal_texture_amount = _amount;
}

std::string Material::getDefinesBlock(){
    std::string defines = "";

    if (m_bEmission) {
        defines += "#define TANGRAM_MATERIAL_EMISSION\n";
        if (m_emission_texture) {
            defines += "#define TANGRAM_MATERIAL_EMISSION_TEXTURE\n";
            if ( m_emission_texture_mapping == MappingType::UV ){
                defines += "#define TANGRAM_MATERIAL_EMISSION_TEXTURE_UV\n";
            } else if ( m_emission_texture_mapping == MappingType::PLANAR ){
                defines += "#define TANGRAM_MATERIAL_EMISSION_TEXTURE_PLANAR\n";
            } else if ( m_emission_texture_mapping == MappingType::TRIPLANAR ){
                defines += "#define TANGRAM_MATERIAL_EMISSION_TEXTURE_TRIPLANAR\n";
            } else if ( m_emission_texture_mapping == MappingType::SPHEREMAP ){
                defines += "#define TANGRAM_MATERIAL_EMISSION_TEXTURE_SPHEREMAP\n";
            }
        }
    }

    if (m_bAmbient) {
        defines += "#define TANGRAM_MATERIAL_AMBIENT\n";
        if (m_ambient_texture) {
            defines += "#define TANGRAM_MATERIAL_AMBIENT_TEXTURE\n";
            if ( m_ambient_texture_mapping == MappingType::UV ){
                defines += "#define TANGRAM_MATERIAL_AMBIENT_TEXTURE_UV\n";
            } else if ( m_ambient_texture_mapping == MappingType::PLANAR ){
                defines += "#define TANGRAM_MATERIAL_AMBIENT_TEXTURE_PLANAR\n";
            } else if ( m_ambient_texture_mapping == MappingType::TRIPLANAR ){
                defines += "#define TANGRAM_MATERIAL_AMBIENT_TEXTURE_TRIPLANAR\n";
            } else if ( m_ambient_texture_mapping == MappingType::SPHEREMAP ){
                defines += "#define TANGRAM_MATERIAL_AMBIENT_TEXTURE_SPHEREMAP\n";
            }
        }
    }
    
    if (m_bDiffuse) {
        defines += "#define TANGRAM_MATERIAL_DIFFUSE\n";
        if (m_diffuse_texture) {
            defines += "#define TANGRAM_MATERIAL_DIFFUSE_TEXTURE\n";
            if ( m_diffuse_texture_mapping == MappingType::UV ){
                defines += "#define TANGRAM_MATERIAL_DIFFUSE_TEXTURE_UV\n";
            } else if ( m_diffuse_texture_mapping == MappingType::PLANAR ){
                defines += "#define TANGRAM_MATERIAL_DIFFUSE_TEXTURE_PLANAR\n";
            } else if ( m_diffuse_texture_mapping == MappingType::TRIPLANAR ){
                defines += "#define TANGRAM_MATERIAL_DIFFUSE_TEXTURE_TRIPLANAR\n";
            } else if ( m_diffuse_texture_mapping == MappingType::SPHEREMAP ){
                defines += "#define TANGRAM_MATERIAL_DIFFUSE_TEXTURE_SPHEREMAP\n";
            }
        }
    }
    
    if (m_bSpecular) {
        defines += "#define TANGRAM_MATERIAL_SPECULAR\n";
        if (m_specular_texture) {
            defines += "#define TANGRAM_MATERIAL_SPECULAR_TEXTURE\n";;
            if ( m_specular_texture_mapping == MappingType::UV ){
                defines += "#define TANGRAM_MATERIAL_SPECULAR_TEXTURE_UV\n";
            } else if ( m_specular_texture_mapping == MappingType::PLANAR ){
                defines += "#define TANGRAM_MATERIAL_SPECULAR_TEXTURE_PLANAR\n";
            } else if ( m_specular_texture_mapping == MappingType::TRIPLANAR ){
                defines += "#define TANGRAM_MATERIAL_SPECULAR_TEXTURE_TRIPLANAR\n";
            } else if ( m_specular_texture_mapping == MappingType::SPHEREMAP ){
                defines += "#define TANGRAM_MATERIAL_SPECULAR_TEXTURE_SPHEREMAP\n";
            }
        }
    }
    
    if (m_normal_texture){

        defines += "#define TANGRAM_MATERIAL_NORMAL_TEXTURE\n";
        if ( m_normal_texture_mapping == MappingType::UV ){
            defines += "#define TANGRAM_MATERIAL_NORMAL_TEXTURE_UV\n";
        } else if ( m_normal_texture_mapping == MappingType::PLANAR ){
            defines += "#define TANGRAM_MATERIAL_NORMAL_TEXTURE_PLANAR\n";
        } else if ( m_normal_texture_mapping == MappingType::TRIPLANAR ){
            defines += "#define TANGRAM_MATERIAL_NORMAL_TEXTURE_TRIPLANAR\n";
        }
    }

    if (m_emission_texture ||  m_ambient_texture || m_diffuse_texture || m_specular_texture ||
        m_normal_texture ) {

        // Add MAPPING functions
        if( (m_bEmission && m_emission_texture_mapping == MappingType::SPHEREMAP) ||
            (m_bAmbient && m_ambient_texture_mapping == MappingType::SPHEREMAP) ||
            (m_bDiffuse && m_diffuse_texture_mapping == MappingType::SPHEREMAP) ||
            (m_bSpecular && m_specular_texture_mapping == MappingType::SPHEREMAP) ||
            m_normal_texture_mapping == MappingType::SPHEREMAP ){
            defines += "#define TANGRAM_MATERIAL_TEXTURE_SPHEREMAP\n";
        }

        if( (m_bEmission && m_emission_texture_mapping == MappingType::TRIPLANAR) ||
            (m_bAmbient && m_ambient_texture_mapping == MappingType::TRIPLANAR) ||
            (m_bDiffuse && m_diffuse_texture_mapping == MappingType::TRIPLANAR) ||
            (m_bSpecular && m_specular_texture_mapping == MappingType::TRIPLANAR) ||
            m_normal_texture_mapping == MappingType::TRIPLANAR ){
            defines += "#define TANGRAM_MATERIAL_TEXTURE_TRIPLANAR\n";
        }

        if( (m_bEmission && m_emission_texture_mapping == MappingType::PLANAR) ||
            (m_bAmbient && m_ambient_texture_mapping == MappingType::PLANAR) ||
            (m_bDiffuse && m_diffuse_texture_mapping == MappingType::PLANAR) ||
            (m_bSpecular && m_specular_texture_mapping == MappingType::PLANAR) ||
            m_normal_texture_mapping == MappingType::PLANAR ){
            defines += "#define TANGRAM_MATERIAL_TEXTURE_PLANAR\n";
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

void Material::removeFromProgram(std::shared_ptr<ShaderProgram> _shader ) {
    _shader->removeSourceBlock("defines", getDefinesBlock());
    _shader->removeSourceBlock("material", getClassBlock());
}

void Material::setupProgram(std::shared_ptr<ShaderProgram> _shader) {

    if (m_bEmission) {
        _shader->setUniformf("u_"+m_name+".emission", m_emission);

        if (m_emission_texture) {
            m_emission_texture->update(1);
            m_emission_texture->bind(1);
            _shader->setUniformi("u_material_emission_texture", 1);
            _shader->setUniformf("u_"+m_name+".emissionScale", m_emission_texture_scale);
        }
    }
    
    if (m_bAmbient) {
        _shader->setUniformf("u_"+m_name+".ambient", m_ambient);

        if (m_ambient_texture) {
            m_ambient_texture->update(2);
            m_ambient_texture->bind(2);
            _shader->setUniformi("u_material_ambient_texture", 2);
            _shader->setUniformf("u_"+m_name+".ambientScale", m_ambient_texture_scale);
        }
    }
    
    if (m_bDiffuse) {
        _shader->setUniformf("u_"+m_name+".diffuse", m_diffuse);

        if (m_diffuse_texture) {
            m_diffuse_texture->update(3);
            m_diffuse_texture->bind(3);
            _shader->setUniformi("u_material_diffuse_texture", 3);
            _shader->setUniformf("u_"+m_name+".diffuseScale", m_diffuse_texture_scale);
        }
    }
    
    if (m_bSpecular) {
        _shader->setUniformf("u_"+m_name+".specular", m_specular);
        _shader->setUniformf("u_"+m_name+".shininess", m_shininess);

        if (m_diffuse_texture) {
            m_diffuse_texture->update(4);
            m_diffuse_texture->bind(4);
            _shader->setUniformi("u_material_specular_texture", 4);
            _shader->setUniformf("u_"+m_name+".specularScale", m_specular_texture_scale);
        }
    }

    if (m_normal_texture) {
        m_normal_texture->update(5);
        m_normal_texture->bind(5);
        _shader->setUniformi("u_material_normal_texture", 5);
        _shader->setUniformf("u_"+m_name+".normalScale", m_normal_texture_scale);
        _shader->setUniformf("u_"+m_name+".normalAmount", m_normal_texture_amount);
    }
}
