#include "shaderProgram.h"

#include "platform.h"
#include "scene/light.h"
#include "gl/disposer.h"
#include "gl/error.h"
#include "gl/renderState.h"
#include "glm/gtc/type_ptr.hpp"
#include "log.h"

#include <sstream>
#include <regex>
#include <set>

namespace Tangram {

ShaderProgram::ShaderProgram() {
    // Nothing to do.
}

ShaderProgram::~ShaderProgram() {

    auto generation = m_generation;
    auto glProgram = m_glProgram;

    m_disposer([=](RenderState& rs) {
        if (rs.isValidGeneration(generation)) {
            if (glProgram != 0) {
                GL::deleteProgram(glProgram);
            }
        }
        // Deleting the shader program that is currently in-use sets the current shader program to 0
        // so we un-set the current program in the render state.
        rs.shaderProgramUnset(glProgram);
    });
}

void ShaderProgram::setSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc){
    m_fragmentShaderSource = std::string(_fragSrc);
    m_vertexShaderSource = std::string(_vertSrc);
    m_needsBuild = true;
}

void ShaderProgram::addSourceBlock(const std::string& _tagName, const std::string& _glslSource, bool _allowDuplicate){

    if (!_allowDuplicate) {
        for (auto& source : m_sourceBlocks[_tagName]) {
            if (_glslSource == source) {
                return;
            }
        }
    }

    size_t start = 0;
    std::string sourceBlock = _glslSource;

    // Certain graphics drivers have issues with shaders having line continuation backslashes "\".
    // Example raster.glsl was having issues on s6 and note2 because of the "\"s in the glsl file.
    // This also makes sure if any "\"s are present in the shaders coming from style sheet will be
    // taken care of.

    // Replace blackslash+newline with spaces (simplification of regex "\\\\\\s*\\n")
    while ((start = sourceBlock.find("\\\n", start)) != std::string::npos) {
        sourceBlock.replace(start, 2, "  ");
        start += 2;
    }

    m_sourceBlocks[_tagName].push_back(sourceBlock);
    m_needsBuild = true;

    //  TODO:
    //          - add Global Blocks
}

GLint ShaderProgram::getAttribLocation(const std::string& _attribName) {

    auto it = m_attribMap.find(_attribName);

    if (it == m_attribMap.end()) {
        // If this is a new entry, get the actual location from OpenGL.
        GLint location = GL::getAttribLocation(m_glProgram, _attribName.c_str());
        m_attribMap[_attribName] = location;
        return location;
    } else {
        return it->second;
    }

}

GLint ShaderProgram::getUniformLocation(const UniformLocation& _uniform) {

    if (m_generation == _uniform.generation) {
        return _uniform.location;
    }

    _uniform.generation = m_generation;
    _uniform.location = GL::getUniformLocation(m_glProgram, _uniform.name.c_str());

    return _uniform.location;
}

bool ShaderProgram::use(RenderState& rs) {
    bool valid = true;

    checkValidity(rs);

    if (m_needsBuild) {
        build(rs);
    }

    valid &= (m_glProgram != 0);

    if (valid) {
        rs.shaderProgram(m_glProgram);
    }

    return valid;
}

bool ShaderProgram::build(RenderState& rs) {

    if (!m_needsBuild) { return false; }
    m_needsBuild = false;
    m_generation = rs.generation();

    // Delete handle for old program; values of 0 are silently ignored
    GL::deleteProgram(m_glProgram);
    m_glProgram = 0;

    // Inject source blocks
    Light::assembleLights(m_sourceBlocks);

    auto vertSrc = applySourceBlocks(m_vertexShaderSource, false);
    auto fragSrc = applySourceBlocks(m_fragmentShaderSource, true);

    // Compile vertex and fragment shaders
    GLint vertexShader = makeCompiledShader(rs, vertSrc, GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        LOGE("Shader compilation failed for %s", m_description.c_str());
        return false;
    }

    GLint fragmentShader = makeCompiledShader(rs, fragSrc, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        LOGE("Shader compilation failed for %s", m_description.c_str());
        return false;
    }

    // Link shaders into a program
    GLint program = makeLinkedShaderProgram(fragmentShader, vertexShader);
    if (program == 0) {
        LOGE("Shader compilation failed for %s", m_description.c_str());
        return false;
    }

    m_glProgram = program;

    // Clear any cached shader locations
    m_attribMap.clear();
    m_disposer = Disposer(rs);

    return true;
}

GLuint ShaderProgram::makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader) {

    GLuint program = GL::createProgram();

    GL::attachShader(program, _fragShader);
    GL::attachShader(program, _vertShader);
    GL::linkProgram(program);

    GLint isLinked;
    GL::getProgramiv(program, GL_LINK_STATUS, &isLinked);

    if (isLinked == GL_FALSE) {
        GLint infoLength = 0;
        GL::getProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);

        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            GL::getProgramInfoLog(program, infoLength, NULL, &infoLog[0]);
            LOGE("linking program:\n%s", &infoLog[0]);
        }

        GL::deleteProgram(program);
        return 0;
    }

    return program;
}

GLuint ShaderProgram::makeCompiledShader(RenderState& rs, const std::string& _src, GLenum _type) {

    auto& cache = (_type == GL_VERTEX_SHADER) ? rs.vertexShaders : rs.fragmentShaders;

    auto entry = cache.emplace(_src, 0);
    if (!entry.second) {
        return entry.first->second;
    }

    GLuint shader = GL::createShader(_type);

    const GLchar* source = (const GLchar*) _src.c_str();
    GL::shaderSource(shader, 1, &source, NULL);
    GL::compileShader(shader);

    GLint isCompiled;
    GL::getShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE) {
        GLint infoLength = 0;
        GL::getShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

        if (infoLength > 1) {
            std::string infoLog;
            infoLog.resize(infoLength);

            GL::getShaderInfoLog(shader, infoLength, NULL, static_cast<GLchar*>(&infoLog[0]));
            LOGE("Shader compilation failed\n%s", infoLog.c_str());

            std::stringstream sourceStream(source);
            std::string item;
            std::vector<std::string> sourceLines;
            while (std::getline(sourceStream, item)) { sourceLines.push_back(item); }

            // Print errors with context
            std::string line;
            std::stringstream logStream(infoLog);

            while (std::getline(logStream, line)) {
                if (line.length() < 2) { continue; }

                int lineNum = 0;
                if (!sscanf(line.c_str(), "%*d%*[(:]%d", &lineNum)) { continue; }
                LOGE("\nError on line %d: %s", lineNum, line.c_str());

                for (int i = std::max(0, lineNum-5); i < lineNum+5; i++) {
                    if (size_t(i) >= sourceLines.size()) { break; }
                    LOGE("%d: %s", i+1, sourceLines[i].c_str());
                }
            }

            // Print full source with line numbers
            LOGD("\n\n");
            for (size_t i = 0; i < sourceLines.size(); i++) {
                LOGD("%d: %s", i, sourceLines[i].c_str());
            }
        }

        GL::deleteShader(shader);
        return 0;
    }

    entry.first->second = shader;

    return shader;
}

std::string ShaderProgram::applySourceBlocks(const std::string& source, bool fragShader) const {

    std::stringstream sourceOut;
    std::set<std::string> pragmas;

    sourceOut << "#define TANGRAM_EPSILON 0.00001\n";
    sourceOut << "#define TANGRAM_WORLD_POSITION_WRAP 100000.\n";

    if (fragShader) {
        sourceOut << "#define TANGRAM_FRAGMENT_SHADER\n";
    } else {
        float depthDelta = 2.f / (1 << 16);
        sourceOut << "#define TANGRAM_DEPTH_DELTA " << std::to_string(depthDelta) << '\n';
        sourceOut << "#define TANGRAM_VERTEX_SHADER\n";
    }

    std::stringstream sourceIn(source);
    std::string line;

    while (std::getline(sourceIn, line)) {
        if (line.empty()) {
            continue;
        }

        sourceOut << line << '\n';

        char pragmaName[128];
        // NB: The initial whitespace is to skip any number of whitespace chars
        if (sscanf(line.c_str(), " #pragma tangram:%127s", pragmaName) == 0) {
            continue;
        }

        auto block = m_sourceBlocks.find(pragmaName);
        if (block == m_sourceBlocks.end()) {
            continue;
        }

        bool unique;
        std::tie(std::ignore, unique) = pragmas.emplace(pragmaName);
        if (!unique) {
            continue;
        }

        // insert blocks
        for (auto& s : block->second) {
            sourceOut << s << '\n';
        }
    }

    // for (auto& block : m_sourceBlocks) {
    //     if (pragmas.find(block.first) == pragmas.end()) {
    //         logMsg("Warning: expected pragma '%s' in shader source\n",
    //                block.first.c_str());
    //     }
    // }

    return sourceOut.str();
}

void ShaderProgram::checkValidity(RenderState& rs) {

    if (!rs.isValidGeneration(m_generation)) {
        m_glProgram = 0;
        m_needsBuild = true;
        m_uniformCache.clear();
    }
}

std::string ShaderProgram::getExtensionDeclaration(const std::string& _extension) {
    std::ostringstream oss;
    oss << "#if defined(GL_ES) == 0 || defined(GL_" << _extension << ")\n";
    oss << "    #extension GL_" << _extension << " : enable\n";
    oss << "    #define TANGRAM_EXTENSION_" << _extension << '\n';
    oss << "#endif\n";
    return oss.str();
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, int _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform1i(location, _value); }
    }
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec2(_value0, _value1));
        if (!cached) { GL::uniform2i(location, _value0, _value1); }
    }
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1, int _value2) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec3(_value0, _value1, _value2));
        if (!cached) { GL::uniform3i(location, _value0, _value1, _value2); }
    }
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1, int _value2, int _value3) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec4(_value0, _value1, _value2, _value3));
        if (!cached) { GL::uniform4i(location, _value0, _value1, _value2, _value3); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, float _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform1f(location, _value); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, float _value0, float _value1) {
    setUniformf(rs, _loc, glm::vec2(_value0, _value1));
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, float _value0, float _value1, float _value2) {
    setUniformf(rs, _loc, glm::vec3(_value0, _value1, _value2));
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, float _value0, float _value1, float _value2, float _value3) {
    setUniformf(rs, _loc, glm::vec4(_value0, _value1, _value2, _value3));
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const glm::vec2& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform2f(location, _value.x, _value.y); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const glm::vec3& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform3f(location, _value.x, _value.y, _value.z); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const glm::vec4& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform4f(location, _value.x, _value.y, _value.z, _value.w); }
    }
}

void ShaderProgram::setUniformMatrix2f(RenderState& rs, const UniformLocation& _loc, const glm::mat2& _value, bool _transpose) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { GL::uniformMatrix2fv(location, 1, _transpose, glm::value_ptr(_value)); }
    }
}

void ShaderProgram::setUniformMatrix3f(RenderState& rs, const UniformLocation& _loc, const glm::mat3& _value, bool _transpose) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { GL::uniformMatrix3fv(location, 1, _transpose, glm::value_ptr(_value)); }
    }
}

void ShaderProgram::setUniformMatrix4f(RenderState& rs, const UniformLocation& _loc, const glm::mat4& _value, bool _transpose) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { GL::uniformMatrix4fv(location, 1, _transpose, glm::value_ptr(_value)); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray1f& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform1fv(location, _value.size(), _value.data()); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray2f& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform2fv(location, _value.size(), (float*)_value.data()); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray3f& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform3fv(location, _value.size(), (float*)_value.data()); }
    }
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, const UniformTextureArray& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL::uniform1iv(location, _value.slots.size(), _value.slots.data()); }
    }
}

}
