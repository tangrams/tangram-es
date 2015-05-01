#include "ambientLight.h"
#include "glm/gtx/string_cast.hpp"

std::string AmbientLight::s_classBlock;
std::string AmbientLight::s_typeName = "AmbientLight";

AmbientLight::AmbientLight(const std::string& _name, bool _dynamic) : 
    Light(_name, _dynamic) {

    m_type = LightType::AMBIENT;
    
}

AmbientLight::~AmbientLight() {

}

void AmbientLight::setupProgram(const std::shared_ptr<View>& _view, std::shared_ptr<ShaderProgram> _shader ) {
    if (m_dynamic) {
        Light::setupProgram(_view, _shader);
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

const std::string& AmbientLight::getTypeName() {

    return s_typeName;

}
