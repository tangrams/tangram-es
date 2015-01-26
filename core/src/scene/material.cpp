#include "material.h"

Material::Material():m_name("material"),
m_emission(0.0),m_ambient(1.0),m_diffuse(0.8),m_specular(0.2),m_shininess(0.2),
m_bEmission(false),m_bAmbient(false),m_bDiffuse(true),m_bSpecular(false) {
    
}

void Material::setEmission(const glm::vec4 _emission){
	m_emission = _emission;
    setEmissionEnabled(true);
}

void Material::setAmbient(const glm::vec4 _ambient){
	m_ambient = _ambient;
    setAmbientEnabled(true);
}

void Material::setDiffuse(const glm::vec4 _diffuse){
	m_diffuse = _diffuse;
    setDiffuseEnabled(true);
}

void Material::setSpecular(const glm::vec4 _specular, float _shinnyFactor){
	m_specular = _specular;
	m_shininess = _shinnyFactor;
    setSpecularEnabled(true);
}

void Material::setEmissionEnabled(bool _enable) { m_bEmission = _enable; }
void Material::setAmbientEnabled(bool _enable) { m_bAmbient = _enable; }
void Material::setDiffuseEnabled(bool _enable) { m_bDiffuse = _enable; }
void Material::setSpecularEnabled(bool _enable) { m_bSpecular = _enable; }

std::string Material::getDefinesBlock(){
	std::string defines = "";

	if (m_bEmission) {
		defines += "#define TANGRAM_MATERIAL_EMISSION\n";
	}
	
	if (m_bAmbient) {
		defines += "#define TANGRAM_MATERIAL_AMBIENT\n";
	}
    
    if (m_bDiffuse) {
    	defines += "#define TANGRAM_MATERIAL_DIFFUSE\n";
    }
    
    if (m_bSpecular) {
    	defines += "#define TANGRAM_MATERIAL_SPECULAR\n";
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
		_shader->setUniformf("u_"+m_name+".emission", m_emission);
	}
	
	if (m_bAmbient) {
		_shader->setUniformf("u_"+m_name+".ambient", m_ambient);
	}
    
    if (m_bDiffuse) {
    	_shader->setUniformf("u_"+m_name+".diffuse", m_diffuse);
    }
    
    if (m_bSpecular) {
    	_shader->setUniformf("u_"+m_name+".specular", m_specular);
    	_shader->setUniformf("u_"+m_name+".shininess", m_shininess);
    }
}