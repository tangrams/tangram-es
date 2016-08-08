#include "light.h"

#include "glm/gtx/string_cast.hpp"
#include "platform.h"
#include "gl/shaderProgram.h"

#include "shaders/lights_glsl.h"

#include <sstream>

namespace Tangram {

Light::Light(const std::string& _name, bool _dynamic):
    m_name(_name),
    m_ambient(0.0f),
    m_diffuse(1.0f),
    m_specular(0.0f),
    m_type(LightType::ambient),
    m_origin(LightOrigin::camera),
    m_dynamic(_dynamic) {
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

void Light::setOrigin(LightOrigin origin) {
    m_dynamic = true;
    m_origin = origin;
}

void Light::injectSourceBlocks(ShaderProgram& _shader) {

    // Inject all needed #defines for this light instance
    _shader.addSourceBlock("defines", getInstanceDefinesBlock(), false);

    if (m_dynamic) {
        // If the light is dynamic, initialize it using the corresponding uniform at the start of main()
        _shader.addSourceBlock("setup", getInstanceName() + " = " + getUniformName() + ";", false);
    }

    _shader.addSourceBlock("__lighting", getClassBlock(), false);
    _shader.addSourceBlock("__lighting", getInstanceBlock());
    _shader.addSourceBlock("__lights_to_compute", getInstanceComputeBlock());
}

void Light::setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms) {
    _uniforms.shader.setUniformf(rs, _uniforms.ambient, m_ambient);
    _uniforms.shader.setUniformf(rs, _uniforms.diffuse, m_diffuse);
    _uniforms.shader.setUniformf(rs, _uniforms.specular, m_specular);
}

void Light::assembleLights(std::map<std::string, std::vector<std::string>>& _sourceBlocks) {

    // Create strings to contain the assembled lighting source code
    std::stringstream lighting;

    // Concatenate all strings at the "__lighting" keys
    // (struct definitions and function definitions)
    for (const auto& string : _sourceBlocks["__lighting"]) {
        lighting << '\n';
        lighting << string;
    }

    // After lights definitions are all added, add the main lighting functions
    std::string lightingBlock = SHADER_SOURCE(lights_glsl);

    // The main lighting functions each contain a tag where all light instances should be computed;
    // Insert all of our "lights_to_compute" at this tag

    std::string tag = "#pragma tangram: lights_to_compute";
    std::stringstream lights;
    for (const auto& string : _sourceBlocks["__lights_to_compute"]) {
        lights << '\n';
        lights << string;
    }

    size_t pos = lightingBlock.find(tag) + tag.length();
    lightingBlock.insert(pos, lights.str());

    // Place our assembled lighting source code back into the map of "source blocks";
    // The assembled strings will then be injected into a shader at the "vertex_lighting"
    // and "fragment_lighting" tags
    _sourceBlocks["lighting"] = { lighting.str() + lightingBlock  };
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
    const std::string& typeName = getTypeName();
    if (m_dynamic) {
        //  If is dynamic, define the uniform and copy it to the global instance of the light struct
        block += "uniform " + typeName + " " + getUniformName() + ";\n";
        block += typeName + " " + getInstanceName() + ";\n";
    } else {
        //  If is not dynamic define the global instance of the light struct and fill the variables
        block += typeName + " " + getInstanceName() + getInstanceAssignBlock() +";\n";
    }
    return block;
}

std::string Light::getInstanceAssignBlock() {
    std::string block = "";
    const std::string& typeName = getTypeName();
    if (!m_dynamic) {
        block += " = " + typeName + "(" + glm::to_string(m_ambient);
        block += ", " + glm::to_string(m_diffuse);
        block += ", " + glm::to_string(m_specular);
    }
    return block;
}

std::string Light::getInstanceComputeBlock() {
    return  "calculateLight(" + getInstanceName() + ", _eyeToPoint, _normal);\n";
}

}
