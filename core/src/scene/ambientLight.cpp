#include "ambientLight.h"
#include "glm/gtx/string_cast.hpp"

std::string AmbientLight::s_classBlock;

AmbientLight::AmbientLight(const std::string& _name, bool _dynamic):Light(_name,_dynamic){
    m_typeName = "AmbientLight";
    m_type = LightType::AMBIENT;
}

AmbientLight::~AmbientLight() {

}

void AmbientLight::setupProgram( std::shared_ptr<ShaderProgram> _shader ) {
    if (m_dynamic) {
        Light::setupProgram(_shader);
    }
}

std::string AmbientLight::getClassBlock() {
    if (s_classBlock.empty()) {
        s_classBlock = stringFromResource("ambientLight.glsl")+"\n";
    }
    return s_classBlock;
}

std::string AmbientLight::getInstanceDefinesBlock() {
    //  Ambient lights don't have defines.... yet.
    return "\n";
}

std::string AmbientLight::getInstanceAssignBlock() {
    std::string block = Light::getInstanceAssignBlock();
    if (!m_dynamic) {
        block += ")";
    }
    return block;
}