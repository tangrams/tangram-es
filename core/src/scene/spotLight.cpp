#include "spotLight.h"
#include "util/stringsOp.h"

SpotLight::SpotLight(const std::string& _name, bool _dynamic):PointLight(_name,_dynamic),m_direction(1.0,0.0,0.0),m_spotExponent(0.0),m_spotCutoff(0.0),m_spotCosCutoff(0.0){
    m_typeName = "SpotLight";
    m_type = LIGHT_SPOT;
}

SpotLight::~SpotLight(){

}

void SpotLight::setDirection(const glm::vec3 &_dir){
    m_direction = _dir;
}

void SpotLight::setCutOff(float _cutoff, float _exponent){
    m_spotCutoff = _cutoff;
    m_spotCosCutoff = cos(_cutoff);
    m_spotExponent = _exponent;
}

void SpotLight::setupProgram( ShaderProgram &_shader ){
    if(m_dynamic){
        PointLight::setupProgram(_shader);
        _shader.setUniformf(getUniformName()+".direction", m_direction);
        _shader.setUniformf(getUniformName()+".spotCosCutoff", m_spotCosCutoff);
        _shader.setUniformf(getUniformName()+".spotExponent", m_spotExponent);
    }
}

std::string SpotLight::getArrayDefinesBlock(int _numberOfLights){
    return "#define NUM_SPOT_LIGHTS " + getString(_numberOfLights) + "\n";
}

std::string SpotLight::getArrayUniformBlock(){
    return "uniform SpotLight u_spotLights[NUM_SPOT_LIGHTS];\n";
}

std::string SpotLight::getClassBlock(){
    return stringFromResource("spot_light.glsl")+"\n";
}

std::string SpotLight::getInstanceDefinesBlock(){
        std::string defines = "\n";

    if(m_constantAttenuation!=0.0){
        defines += "#ifndef SPOTLIGHT_CONSTANT_ATTENUATION\n";
        defines += "#define SPOTLIGHT_CONSTANT_ATTENUATION\n";
        defines += "#endif\n";
    }

    if(m_linearAttenuation!=0.0){
        defines += "#ifndef SPOTLIGHT_LINEAR_ATTENUATION\n";
        defines += "#define SPOTLIGHT_LINEAR_ATTENUATION\n";
        defines += "#endif\n";
    }

    if(m_quadraticAttenuation!=0.0){
        defines += "#ifndef SPOTLIGHT_QUADRATIC_ATTENUATION\n";
        defines += "#define SPOTLIGHT_QUADRATIC_ATTENUATION\n";
        defines += "#endif\n";
    }
    return defines;
}

std::string SpotLight::getInstanceAssignBlock(){
    std::string block = PointLight::getInstanceAssignBlock();
    if(!m_dynamic){
        block += getInstanceName() + ".direction = " + getString(m_direction) + ";\n";
        block += getInstanceName() + ".spotCosCutoff = " + getString(m_spotCosCutoff) + ";\n";
        block += getInstanceName() + ".spotExponent = " + getString(m_spotExponent) + ";\n";
    }
    return block;
}
