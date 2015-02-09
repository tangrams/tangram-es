#include "light.h"
#include "glm/gtx/string_cast.hpp"

std::string Light::s_vertexLightingBlock;
std::string Light::s_fragmentLightingBlock;

Light::Light(const std::string& _name, bool _dynamic):
    m_name(_name),
    m_typeName("undefined_light"),
    m_ambient(0.0f),
    m_diffuse(1.0f),
    m_specular(0.0f),
    m_type(LightType::UNDEFINED),
    m_injType(VERTEX),
    m_dynamic(_dynamic) {

}

Light::~Light() {

}

void Light::setName(const std::string &_name) {
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

void Light::injectOnProgram(std::shared_ptr<ShaderProgram> _shader, InjectionType _injType) {

    if (_injType != DEFAULT) {
        m_injType = _injType;
    }
    
    // Inject all needed #defines for this light instance
    
    _shader->addSourceBlock("defines", "#define TANGRAM_LIGHTS\n", false);
    _shader->addSourceBlock("defines", getInstanceDefinesBlock(), false);
    if (m_injType == FRAGMENT) {
        _shader->addSourceBlock("defines", "#define TANGRAM_FRAGMENT_LIGHTS\n", false);
    }
    if (m_injType == VERTEX) {
        _shader->addSourceBlock("defines", "#define TANGRAM_VERTEX_LIGHTS\n", false);
    }
    
    // For the vertex and/or fragment shaders (based on specified injection type), inject:
    // 1. The struct and function definitions for this light class ("ClassBlock")
    // 2. The declaration of this light instance ("InstanceBlock")
    // 3. The function call to compute lighting for this light instance ("InstanceComputeBlock")
    //
    // These source strings are added to tags which do not directly correspond to a tag in the
    // actual shader source, instead these tags are referenced in the <assembleLights> function
    // to build a single string with all the lighting source code in a usable order.
    
    if (m_injType == FRAGMENT) {
        _shader->addSourceBlock("__fragment_lighting", getClassBlock(), false);
        _shader->addSourceBlock("__fragment_lighting", getInstanceBlock());
        _shader->addSourceBlock("__fragment_lights_to_compute", getInstanceComputeBlock());
    }

    if (m_injType == VERTEX) {
        _shader->addSourceBlock("__vertex_lighting", getClassBlock(), false);
        _shader->addSourceBlock("__vertex_lighting", getInstanceBlock());
        _shader->addSourceBlock("__vertex_lights_to_compute", getInstanceComputeBlock());
    }
}

void Light::setupProgram(std::shared_ptr<ShaderProgram> _shader) {
    if (m_dynamic) {
        _shader->setUniformf(getUniformName()+".ambient", m_ambient);
        _shader->setUniformf(getUniformName()+".diffuse", m_diffuse);
        _shader->setUniformf(getUniformName()+".specular", m_specular);
    }
}

void Light::assembleLights(std::map<std::string, std::vector<std::string>>& _sourceBlocks) {
    
    // If needed, load the strings containing the main vertex and fragment lighting functions
    
    if (s_vertexLightingBlock.empty()) {
        s_vertexLightingBlock = stringFromResource("lightVertex.glsl");
    }
    if (s_fragmentLightingBlock.empty()) {
        s_fragmentLightingBlock = stringFromResource("lightFragment.glsl");
    }
    
    // Create strings to contain the assembled vertex and fragment lighting source code
    
    std::string vertexLighting;
    std::string fragmentLighting;
    
    // Concatenate all strings at the "__vertex_lighting" and "__fragment_lighting" keys
    // (struct definitions and function definitions)
    
    for (const auto& string : _sourceBlocks["__vertex_lighting"]) {
        vertexLighting += string;
    }
    for (const auto& string : _sourceBlocks["__fragment_lighting"]) {
        fragmentLighting += string;
    }
    
    // After lights definitions are all added, add the main lighting functions
    
    vertexLighting += s_vertexLightingBlock;
    fragmentLighting += s_fragmentLightingBlock;
    
    // The main lighting functions each contain a tag where all light instances should be computed;
    // Insert all of our "lights_to_compute" at this tag
    
    std::string tag = "#pragma tangram: vertex_lights_to_compute";
    size_t pos = vertexLighting.find(tag) + tag.length();
    for (const auto& string : _sourceBlocks["__vertex_lights_to_compute"]) {
        vertexLighting.insert(pos, string);
        pos += string.length();
    }
    
    tag = "#pragma tangram: fragment_lights_to_compute";
    pos = fragmentLighting.find(tag) + tag.length();
    for (const auto& string : _sourceBlocks["__fragment_lights_to_compute"]) {
        fragmentLighting.insert(pos, string);
        pos += string.length();
    }
    
    // Place our assembled lighting source code back into the map of "source blocks";
    // The assembled strings will then be injected into a shader at the "vertex_lighting"
    // and "fragment_lighting" tags
    
    _sourceBlocks["vertex_lighting"] = { vertexLighting };
    _sourceBlocks["fragment_lighting"]= { fragmentLighting };
    
}

std::string Light::getName() {
	return m_name;
}

LightType Light::getType() {
    return m_type;
}

InjectionType Light::getInjectionType() {
    return m_injType;
}

std::string Light::getUniformName() {
	return "u_" + m_name;
}

std::string Light::getInstanceName() {
    return "g_" + m_name;
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
