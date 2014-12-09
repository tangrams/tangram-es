#include "material.h"

#define STRINGIFY(A) #A

Material::Material():m_name("material"),
m_emission(0.0),m_ambient(1.0),m_diffuse(0.8),m_specular(0.2),m_shininess(0.2),
m_bEmission(false),m_bAmbient(false),m_bDiffuse(true),m_bSpecular(true){
    
}

void Material::setEmission(const glm::vec4 _emission){
	m_emission = _emission;
	if(!m_bEmission){
		enableEmission();
	}
}

void Material::setAmbient(const glm::vec4 _ambient){
	m_ambient = _ambient;
	if(!m_bAmbient){
		enableAmbient();
	}
}

void Material::setDiffuse(const glm::vec4 _diffuse){
	m_diffuse = _diffuse;
	if(!m_bDiffuse){
		enableDiffuse();
	}
}

void Material::setSpecular(const glm::vec4 _specular, float _shinnyFactor){
	m_specular = _specular;
	m_shininess = _shinnyFactor;

	if(!m_bSpecular){
		enableSpecular();
	}
}

void Material::enableEmission(){ m_bEmission = true; }
void Material::disableEmission(){ m_bEmission = false; }

void Material::enableAmbient(){ m_bAmbient = true; };
void Material::disableAmbient(){ m_bAmbient = false; };

void Material::enableDiffuse(){ m_bDiffuse = true; };
void Material::disableDiffuse(){ m_bDiffuse = false; };

void Material::enableSpecular(){ m_bSpecular = true; };
void Material::disableSpecular(){ m_bSpecular = false; };

std::string Material::getTransform(){
	std::string defines = "\n";

	if(m_bEmission){
		defines += "#define MATERIAL_EMISSION\n";
	}
	
	if(m_bAmbient){
		defines += "#define MATERIAL_AMBIENT\n";
	}
    
    if(m_bDiffuse){
    	defines += "#define MATERIAL_DIFFUSE\n";
    }
    
    if(m_bSpecular){
    	defines += "#define MATERIAL_SPECULAR\n";
    }

    return defines + stringFromResource("material.glsl") + "\n";
}

void Material::setupProgram(ShaderProgram &_shader){
	if(m_bEmission){
		_shader.setUniformf("u_"+m_name+".emission", m_emission);
	}
	
	if(m_bAmbient){
		_shader.setUniformf("u_"+m_name+".ambient", m_ambient);
	}
    
    if(m_bDiffuse){
    	_shader.setUniformf("u_"+m_name+".diffuse", m_diffuse);
    }
    
    if(m_bSpecular){
    	_shader.setUniformf("u_"+m_name+".specular", m_specular);
    	_shader.setUniformf("u_"+m_name+".shininess", m_shininess);
    }
}