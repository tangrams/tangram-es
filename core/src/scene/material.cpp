#include "material.h"

#define STRINGIFY(A) #A

Material::Material():m_name("material"),
m_emission(0.0),m_ambient(1.0),m_diffuse(0.8),m_specular(0.2),m_shininess(0.2),
m_bEmission(false),m_bAmbient(false),m_bDiffuse(true),m_bSpecular(false){
    
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
void Material::enableAmbient(){ m_bAmbient = true; };
void Material::enableDiffuse(){ m_bDiffuse = true; };
void Material::enableSpecular(){ m_bSpecular = true; };

std::string Material::getDefinesBlock(){
	std::string defines = "\n";

	if(m_bEmission){
		defines += "#ifndef MATERIAL_EMISSION\n";
		defines += "#define MATERIAL_EMISSION\n";
		defines	+= "#endif\n";
	}
	
	if(m_bAmbient){
		defines += "#ifndef MATERIAL_AMBIENT\n";
		defines += "#define MATERIAL_AMBIENT\n";
		defines	+= "#endif\n";
	}
    
    if(m_bDiffuse){
    	defines += "#ifndef MATERIAL_DIFFUSE\n";
    	defines += "#define MATERIAL_DIFFUSE\n";
    	defines	+= "#endif\n";
    }
    
    if(m_bSpecular){
    	defines += "#ifndef MATERIAL_SPECULAR\n";
    	defines += "#define MATERIAL_SPECULAR\n";
    	defines	+= "#endif\n";
    }
    
    return defines;
}

std::string Material::getClassBlock(){
	return stringFromResource("material.glsl") + "\n";
}

// std::string Material::getInstanceBlock(){
//     std::string block = "";
//     if(m_dynamic){    
//         //  If is dynamic, define the uniform and copy it to the global instance of the light struct
//         block += "uniform " + m_typeName + " " + getUniformName() + ";\n";
//         block += m_typeName + " " + getInstanceName() + " = " + getUniformName() + ";\n";
//     } else {
//         //  If is not dynamic define the global instance of the light struct and fill the variables
//         block += m_typeName + " " + getInstanceName() + getInstanceAssignBlock() +";\n";
//     }
//     return block;
// }

// std::string Material::getInstanceAssignBlock(){
//     std::string block = "";
//     if(!m_dynamic){
//         // block += getInstanceName() + ".ambient = " + getString(m_ambient) + ";\n";
//         // block += getInstanceName() + ".diffuse = " + getString(m_diffuse) + ";\n";
//         // block += getInstanceName() + ".specular = " + getString(m_specular) + ";\n";

//         block += " = " + m_typeName + "(" + getString(m_ambient);
//         block += ", " + getString(m_diffuse);
//         block += ", " + getString(m_specular);
//     }
//     return block;
// }

void Material::injectOnProgram( std::shared_ptr<ShaderProgram> _shader ){
	//  Each light will add the needed :
    _shader->addSourceBlock("defines",    getDefinesBlock());
    _shader->addSourceBlock("material",   getClassBlock() );
}

void Material::setupProgram(std::shared_ptr<ShaderProgram> _shader){
	if(m_bEmission){
		_shader->setUniformf("u_"+m_name+".emission", m_emission);
	}
	
	if(m_bAmbient){
		_shader->setUniformf("u_"+m_name+".ambient", m_ambient);
	}
    
    if(m_bDiffuse){
    	_shader->setUniformf("u_"+m_name+".diffuse", m_diffuse);
    }
    
    if(m_bSpecular){
    	_shader->setUniformf("u_"+m_name+".specular", m_specular);
    	_shader->setUniformf("u_"+m_name+".shininess", m_shininess);
    }
}