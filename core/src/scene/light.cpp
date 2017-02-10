#include "scene/light.h"

#include "gl/shaderProgram.h"
#include "lights_glsl.h"
#include "platform.h"

#include "glm/gtx/string_cast.hpp"
#include <sstream>
#include <set>

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


void Light::setupProgram(RenderState& rs, const View& _view, ShaderProgram& _shader,
                         LightUniforms& _uniforms) {
    _shader.setUniformf(rs, _uniforms.ambient, m_ambient);
    _shader.setUniformf(rs, _uniforms.diffuse, m_diffuse);
    _shader.setUniformf(rs, _uniforms.specular, m_specular);
}

auto Light::assembleLights(const std::vector<std::unique_ptr<Light>>& _lights) ->
    std::map<std::string, std::string> {

    // Create strings to contain the assembled lighting source code
    std::stringstream lighting;

    std::map<std::string, std::string> sourceBlocks;

    std::set<std::string> lightClasses;
    std::set<std::string> lightDefines;
    std::set<std::string> lightSetup;

    for (auto& light : _lights) {
        lightDefines.insert(light->getInstanceDefinesBlock());
        lightClasses.insert(light->getClassBlock());
        if (light->isDynamic()) {
            lightSetup.insert(light->getInstanceName() + " = " + light->getUniformName() + ";");
        }
    }

    for (auto& string: lightClasses) {
        lighting << '\n' << string;
    }

    std::stringstream definesBlock;
    for (auto& string: lightDefines) {
        definesBlock << '\n' << string;
    }
    sourceBlocks["defines"] = definesBlock.str();

    std::stringstream setupBlock;
    for (auto& string: lightSetup) {
        setupBlock << '\n' << string;
    }
    sourceBlocks["setup"] = setupBlock.str();

    for (auto& light : _lights) {
        lighting << '\n' << light->getInstanceBlock();
    }
    // After lights definitions are all added, add the main lighting functions
    std::string lightingBlock = SHADER_SOURCE(lights_glsl);

    // The main lighting functions each contain a tag where all light instances should be computed;
    std::stringstream lights;
    for (auto& light : _lights) {
        lights << '\n' << light->getInstanceComputeBlock();
    }

    const std::string tag = "#pragma tangram: lights_to_compute";
    size_t pos = lightingBlock.find(tag) + tag.length();
    lightingBlock.insert(pos, lights.str());

    // Place our assembled lighting source code back into the map of "source blocks";
    // The assembled strings will then be injected into a shader at the "vertex_lighting"
    // and "fragment_lighting" tags
    sourceBlocks["lighting"] = lighting.str() + lightingBlock;

    return sourceBlocks;
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
