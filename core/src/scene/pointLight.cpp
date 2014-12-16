#include "pointLight.h"
#include "util/stringsOp.h"

PointLight::PointLight(const std::string& _name, bool _dynamic):Light(_name,_dynamic),m_position(0.0),m_constantAttenuation(0.0),m_linearAttenuation(0.0),m_quadraticAttenuation(0.0){
    m_typeName = "PointLight";
    m_type = LightType::LIGHT_POINT;
}

PointLight::~PointLight(){

}

void PointLight::setPosition(const glm::vec3 &_pos){
    m_position.x = _pos.x;
    m_position.y = _pos.y;
    m_position.z = _pos.z;
    m_position.w = 0.0;
}

void PointLight::setConstantAttenuation(float _constantAtt){
    m_constantAttenuation = _constantAtt;
}

void PointLight::setLinearAttenuation(float _linearAtt){
    m_linearAttenuation = _linearAtt;
}

void PointLight::setQuadreaticAttenuation(float _quadraticAtt){
    m_quadraticAttenuation = _quadraticAtt;
}

void PointLight::setAttenuation(float _constant, float _linear, float _quadratic){
    m_constantAttenuation = _constant;
    m_linearAttenuation = _linear;
    m_quadraticAttenuation = _quadratic;
}

void PointLight::setupProgram( ShaderProgram* _shader ){
    if(m_dynamic){
        Light::setupProgram(_shader);
        _shader->setUniformf(getUniformName()+".position", glm::vec4(m_position) );

        if(m_constantAttenuation!=0.0){
            _shader->setUniformf(getUniformName()+".constantAttenuation", m_constantAttenuation);
        }

        if(m_linearAttenuation!=0.0){
            _shader->setUniformf(getUniformName()+".linearAttenuation", m_linearAttenuation);
        }

        if(m_quadraticAttenuation!=0.0){
            _shader->setUniformf(getUniformName()+".quadraticAttenuation", m_quadraticAttenuation);
        }
    }
}

std::string PointLight::getClassBlock(){
    return stringFromResource("point_light.glsl")+"\n";
}

std::string PointLight::getInstanceDefinesBlock(){
    std::string defines = "\n";

    if(m_constantAttenuation!=0.0){
        defines += "#ifndef POINTLIGHT_CONSTANT_ATTENUATION\n";
        defines += "#define POINTLIGHT_CONSTANT_ATTENUATION\n";
        defines += "#endif\n";
    }

    if(m_linearAttenuation!=0.0){
        defines += "#ifndef POINTLIGHT_LINEAR_ATTENUATION\n";
        defines += "#define POINTLIGHT_LINEAR_ATTENUATION\n";
        defines += "#endif\n";
    }

    if(m_quadraticAttenuation!=0.0){
        defines += "#ifndef POINTLIGHT_QUADRATIC_ATTENUATION\n";
        defines += "#define POINTLIGHT_QUADRATIC_ATTENUATION\n";
        defines += "#endif\n";
    }
    return defines;
}

std::string PointLight::getInstanceAssignBlock(){
    std::string block = Light::getInstanceAssignBlock();
    if(!m_dynamic){
        block += getInstanceName() + ".position = " + getString(m_position) + ";\n";
        if(m_constantAttenuation!=0.0){
            block += getInstanceName() + ".constantAttenuation = " + getString(m_constantAttenuation) + ";\n";
        }
        if(m_linearAttenuation!=0.0){
            block += getInstanceName() + ".linearAttenuation = " + getString(m_linearAttenuation) + ";\n";
        }
        if(m_quadraticAttenuation!=0.0){
            block += getInstanceName() + ".quadraticAttenuation = " + getString(m_quadraticAttenuation) + ";\n";
        }
    }
    return block;
}
