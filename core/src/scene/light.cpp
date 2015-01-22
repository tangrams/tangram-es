#include "light.h"

#include "util/stringsOp.h"

std::string Light::s_vertexLightingBlock;
std::string Light::s_fragmentLightingBlock;

Light::Light(const std::string& _name, bool _dynamic):m_name(_name),m_typeName("undefined_light"),m_ambient(0.0f),m_diffuse(1.0f),m_specular(0.0f),m_type(LightType::UNDEFINED),m_injType(VERTEX),m_dynamic(_dynamic) {

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
    
    _shader->addSourceBlock("defines", "#define TANGRAM_LIGHTS", false);
    _shader->addSourceBlock("defines", getInstanceDefinesBlock(), false);

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
    
    if (s_vertexLightingBlock.empty()) {
        s_vertexLightingBlock = stringFromResource("lightVertex.glsl");
    }
    
    if (s_fragmentLightingBlock.empty()) {
        s_fragmentLightingBlock = stringFromResource("lightFragment.glsl");
    }
    
    std::string vertexLighting;
    std::string fragmentLighting;
    
    for (const auto& string : _sourceBlocks["__vertex_lighting"]) {
        vertexLighting += string;
    }
    
    for (const auto& string : _sourceBlocks["__fragment_lighting"]) {
        fragmentLighting += string;
    }
    
    vertexLighting += s_vertexLightingBlock;
    fragmentLighting += s_fragmentLightingBlock;
    
    std::string tag = "#pragma tangram: vertex_lights_to_compute";
    int pos = vertexLighting.find(tag) + tag.length();
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
    
    _sourceBlocks["vertex_lighting"].push_back(vertexLighting);
    _sourceBlocks["fragment_lighting"].push_back(fragmentLighting);
    
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
        block += " = " + m_typeName + "(" + getString(m_ambient);
        block += ", " + getString(m_diffuse);
        block += ", " + getString(m_specular);
    }
    return block;
}

std::string Light::getInstanceComputeBlock() {
    return "calculateLight("+getInstanceName()+", _eyeToPoint, _normal);\n";
}
