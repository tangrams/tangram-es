#include "light.h"
#include "glm/gtx/string_cast.hpp"

std::string Light::s_lightToCalculate;

Light::Light(const std::string& _name, bool _dynamic):
    m_name(_name),
    m_typeName("undefined_light"),
    m_ambient(0.0f),
    m_diffuse(1.0f),
    m_specular(0.0f),
    m_type(LightType::UNDEFINED),
    m_origin(LightOrigin::CAMERA),
    m_dynamic(_dynamic){
}

Light::~Light() {

}

void Light::setInstanceName(const std::string &_name) {
    m_name = _name;
}

void Light::setAmbientColor(const glm::vec4 _ambient) {
    m_ambient = _ambient;
}

void Light::setDiffuseColor(const glm::vec4 _diffuse) {
    m_diffuse = _diffuse;
}

void Light::setSpecularColor(const glm::vec4 _specular) {
    m_specular = _specular;
}

void Light::setOrigin( LightOrigin _origin ){
    m_origin = _origin;
}

void Light::injectOnProgram(std::shared_ptr<ShaderProgram> _shader) {
    
    // Inject all needed #defines for this light instance
    _shader->addSourceBlock("defines", getInstanceDefinesBlock(), false);

    _shader->addSourceBlock("__lighting", getClassBlock(), false);
    _shader->addSourceBlock("__lighting", getInstanceBlock());
    _shader->addSourceBlock("__lights_to_compute", getInstanceComputeBlock());

    s_lightToCalculate += getInstanceComputeBlock();
}

void Light::setupProgram(std::shared_ptr<ShaderProgram> _shader) {
    if (m_dynamic) {
        _shader->setUniformf(getUniformName()+".ambient", m_ambient);
        _shader->setUniformf(getUniformName()+".diffuse", m_diffuse);
        _shader->setUniformf(getUniformName()+".specular", m_specular);
    }
}

void Light::assembleLights(std::map<std::string, std::vector<std::string>>& _sourceBlocks) {
    
    // Create strings to contain the assembled lighting source code
    std::string lightingBlock;
    
    // Concatenate all strings at the "__lighting" keys
    // (struct definitions and function definitions)
    
    for (const auto& string : _sourceBlocks["__lighting"]) {
        lightingBlock += string;
    }
    
    // After lights definitions are all added, add the main lighting functions
    lightingBlock += stringFromResource("lights.glsl");

    // The main lighting functions each contain a tag where all light instances should be computed;
    // Insert all of our "lights_to_compute" at this tag
    
    std::string tag = "#pragma tangram: lights_to_compute";
    size_t pos = lightingBlock.find(tag) + tag.length();
    for (const auto& string : _sourceBlocks["__lights_to_compute"]) {
        lightingBlock.insert(pos, string);
        pos += string.length();
    }
    
    // Place our assembled lighting source code back into the map of "source blocks";
    // The assembled strings will then be injected into a shader at the "vertex_lighting"
    // and "fragment_lighting" tags
    _sourceBlocks["lighting"] = { lightingBlock };
}

LightType Light::getType() {
    return m_type;
}

std::string Light::getUniformName() {
	return "u_" + m_name;
}

std::string Light::getInstanceName() {
    return m_name;
}

std::string Light::getInstanceBlock() {
    std::string block = "";
    if (m_dynamic) {
        //  If is dynamic, define the uniform and copy it to the global instance of the light struct
        block += "uniform " + m_typeName + " " + getUniformName() + ";\n";
        block += m_typeName + " " + getInstanceName() + " = " + getUniformName() + ";\n";
    } else {
        //  If is not dynamic define the global instance of the light struct and fill the variables
        block += m_typeName + " " + getInstanceName() + getInstanceAssignBlock() +";\n";
    }
    return block;
}

std::string Light::getInstanceAssignBlock() {
    std::string block = "";
    if (!m_dynamic) {
        block += " = " + m_typeName + "(" + glm::to_string(m_ambient);
        block += ", " + glm::to_string(m_diffuse);
        block += ", " + glm::to_string(m_specular);
    }
    return block;
}

std::string Light::getInstanceComputeBlock() {
    return "calculateLight("+getInstanceName()+", _eyeToPoint, _normal);\n";
}
