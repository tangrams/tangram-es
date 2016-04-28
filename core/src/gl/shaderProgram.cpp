#include "shaderProgram.h"

#include "platform.h"
#include "scene/light.h"
#include "gl/renderState.h"
#include "glm/gtc/type_ptr.hpp"

#include <sstream>
#include <regex>
#include <set>

namespace Tangram {


ShaderProgram::ShaderProgram() {

    m_glProgram = 0;
    m_glFragmentShader = 0;
    m_glVertexShader = 0;
    m_needsBuild = true;
    m_generation = -1;
    m_invalidShaderSource = false;
    m_description = "";
}

ShaderProgram::~ShaderProgram() {

    if (m_glProgram != 0) {
        glDeleteProgram(m_glProgram);
    }

    if (m_glFragmentShader != 0) {
        glDeleteShader(m_glFragmentShader);
    }

    if (m_glVertexShader != 0) {
        glDeleteShader(m_glVertexShader);
    }

    // Deleting a shader program being used ends up setting up the current shader program to 0
    // after the driver finishes using it, force this setup by setting the current program
    if (RenderState::shaderProgram.compare(m_glProgram)) {
        RenderState::shaderProgram.init(0, false);
    }

    m_attribMap.clear();

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

    m_sourceBlocks[_tagName].push_back(_glslSource);
    m_needsBuild = true;

    //  TODO:
    //          - add Global Blocks
}

GLint ShaderProgram::getAttribLocation(const std::string& _attribName) {

    // Get uniform location at this key, or create one valued at -2 if absent
    GLint& location = m_attribMap[_attribName].loc;

    // -2 means this is a new entry
    if (location == -2) {
        // Get the actual location from OpenGL
        location = glGetAttribLocation(m_glProgram, _attribName.c_str());
    }

    return location;

}

GLint ShaderProgram::getUniformLocation(const UniformLocation& _uniform) {

    if (m_generation == _uniform.generation) {
        return _uniform.location;
    }

    _uniform.generation = m_generation;
    _uniform.location = glGetUniformLocation(m_glProgram, _uniform.name.c_str());

    return _uniform.location;
}

bool ShaderProgram::use() {
    bool valid = true;

    checkValidity();

    if (m_needsBuild) {
        build();
    }

    valid &= (m_glProgram != 0);

    if (valid) {
        RenderState::shaderProgram(m_glProgram);
    }

    return valid;
}

bool ShaderProgram::build() {

    m_needsBuild = false;
    m_generation = RenderState::generation();

    if (m_invalidShaderSource) { return false; }

    // Inject source blocks

    Light::assembleLights(m_sourceBlocks);

    auto vertSrc = applySourceBlocks(m_vertexShaderSource, false);
    auto fragSrc = applySourceBlocks(m_fragmentShaderSource, true);

    // Try to compile vertex and fragment shaders, releasing resources and quiting on failure

    GLint vertexShader = makeCompiledShader(vertSrc, GL_VERTEX_SHADER);

    if (vertexShader == 0) {
        return false;
    }

    GLint fragmentShader = makeCompiledShader(fragSrc, GL_FRAGMENT_SHADER);

    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Try to link shaders into a program, releasing resources and quiting on failure

    GLint program = makeLinkedShaderProgram(fragmentShader, vertexShader);

    if (program == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Delete handles for old shaders and program; values of 0 are silently ignored

    glDeleteShader(m_glFragmentShader);
    glDeleteShader(m_glVertexShader);
    glDeleteProgram(m_glProgram);

    m_glFragmentShader = fragmentShader;
    m_glVertexShader = vertexShader;
    m_glProgram = program;

    // Clear any cached shader locations

    m_attribMap.clear();

    return true;
}

GLuint ShaderProgram::makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader) {

    GLuint program = glCreateProgram();
    glAttachShader(program, _fragShader);
    glAttachShader(program, _vertShader);
    glLinkProgram(program);

    GLint isLinked;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);

    if (isLinked == GL_FALSE) {
        GLint infoLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            glGetProgramInfoLog(program, infoLength, NULL, &infoLog[0]);
            LOGE("linking program:\n%s", &infoLog[0]);
        }
        glDeleteProgram(program);
        m_invalidShaderSource = true;
        return 0;
    }

    return program;
}

GLuint ShaderProgram::makeCompiledShader(const std::string& _src, GLenum _type) {

    GLuint shader = glCreateShader(_type);
    const GLchar* source = (const GLchar*) _src.c_str();
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint isCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE) {
        GLint infoLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            glGetShaderInfoLog(shader, infoLength, NULL, &infoLog[0]);
            LOGE("Shader compilation failed %s", m_description.c_str());
            LOGE("%s", &infoLog[0]);
            //logMsg("\n%s\n", source);
        }
        glDeleteShader(shader);
        m_invalidShaderSource = true;
        return 0;
    }

    return shader;

}


std::string ShaderProgram::applySourceBlocks(const std::string& source, bool fragShader) {

    static const std::regex pragmaLine("^\\s*#pragma tangram:\\s+(\\w+).*$");

    std::stringstream sourceOut;
    std::set<std::string> pragmas;
    std::smatch sm;

    sourceOut << "#define TANGRAM_EPSILON 0.00001\n";
    sourceOut << "#define TANGRAM_WORLD_POSITION_WRAP 100000.\n";

    if (fragShader) {
        sourceOut << "#define TANGRAM_FRAGMENT_SHADER\n";
    } else {
        float depthDelta = 2.f / (1 << 16);
        sourceOut << "#define TANGRAM_DEPTH_DELTA " << std::to_string(depthDelta) << '\n';
        sourceOut << "#define TANGRAM_VERTEX_SHADER\n";
    }

    auto sourcePos = source.begin();
    size_t lineStart = 0, lineEnd;

    while ((lineEnd = source.find('\n', lineStart)) != std::string::npos) {

        if (lineEnd - lineStart == 0) {
            // skip empty lines
            lineStart += 1;
            continue;
        }

        auto matchPos = source.begin() + lineStart;
        auto matchEnd = source.begin() + lineEnd;
        lineStart = lineEnd + 1;

        if (std::regex_match(matchPos, matchEnd, sm, pragmaLine)) {

            std::string pragmaName = sm[1];

            bool unique;
            std::tie(std::ignore, unique) = pragmas.emplace(std::move(pragmaName));

            // ignore duplicates
            if (!unique) { continue; }

            auto block = m_sourceBlocks.find(sm[1]);
            if (block == m_sourceBlocks.end()) { continue; }

            // write from last source position to end of pragma
            sourceOut << '\n';
            std::copy(sourcePos, matchEnd, std::ostream_iterator<char>(sourceOut));
            sourcePos = matchEnd;

            // insert blocks
            for (auto& source : block->second) {
                sourceOut << '\n';
                sourceOut << source;
            }
        }
    }

    // write from last written source position to end of source
    std::copy(sourcePos, source.end(), std::ostream_iterator<char>(sourceOut));

    // for (auto& block : m_sourceBlocks) {
    //     if (pragmas.find(block.first) == pragmas.end()) {
    //         logMsg("Warning: expected pragma '%s' in shader source\n",
    //                block.first.c_str());
    //     }
    // }

    // Certain graphics drivers have issues with shaders having line continuation backslashes "\".
    // Example raster.glsl was having issues on s6 and note2 because of the "\"s in the glsl file.
    // This also makes sure if any "\"s are present in the shaders coming from style sheet will be
    // taken care of.
    auto str = sourceOut.str();
    std::regex backslashMatch("\\\\\\s*\\n");

    return std::regex_replace(str, backslashMatch, " ");
}

void ShaderProgram::checkValidity() {

    if (!RenderState::isValidGeneration(m_generation)) {
        m_glFragmentShader = 0;
        m_glVertexShader = 0;
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

void ShaderProgram::setUniformi(const UniformLocation& _loc, int _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform1i(location, _value); }
    }
}

void ShaderProgram::setUniformi(const UniformLocation& _loc, int _value0, int _value1) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec2(_value0, _value1));
        if (!cached) { glUniform2i(location, _value0, _value1); }
    }
}

void ShaderProgram::setUniformi(const UniformLocation& _loc, int _value0, int _value1, int _value2) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec3(_value0, _value1, _value2));
        if (!cached) { glUniform3i(location, _value0, _value1, _value2); }
    }
}

void ShaderProgram::setUniformi(const UniformLocation& _loc, int _value0, int _value1, int _value2, int _value3) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec4(_value0, _value1, _value2, _value3));
        if (!cached) { glUniform4i(location, _value0, _value1, _value2, _value3); }
    }
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, float _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform1f(location, _value); }
    }
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, float _value0, float _value1) {
    setUniformf(_loc, glm::vec2(_value0, _value1));
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, float _value0, float _value1, float _value2) {
    setUniformf(_loc, glm::vec3(_value0, _value1, _value2));
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, float _value0, float _value1, float _value2, float _value3) {
    setUniformf(_loc, glm::vec4(_value0, _value1, _value2, _value3));
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, const glm::vec2& _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform2f(location, _value.x, _value.y); }
    }
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, const glm::vec3& _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform3f(location, _value.x, _value.y, _value.z); }
    }
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, const glm::vec4& _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform4f(location, _value.x, _value.y, _value.z, _value.w); }
    }
}

void ShaderProgram::setUniformMatrix2f(const UniformLocation& _loc, const glm::mat2& _value, bool _transpose) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { glUniformMatrix2fv(location, 1, _transpose, glm::value_ptr(_value)); }
    }
}

void ShaderProgram::setUniformMatrix3f(const UniformLocation& _loc, const glm::mat3& _value, bool _transpose) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { glUniformMatrix3fv(location, 1, _transpose, glm::value_ptr(_value)); }
    }
}

void ShaderProgram::setUniformMatrix4f(const UniformLocation& _loc, const glm::mat4& _value, bool _transpose) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { glUniformMatrix4fv(location, 1, _transpose, glm::value_ptr(_value)); }
    }
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, const UniformArray1f& _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform1fv(location, _value.size(), _value.data()); }
    }
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, const UniformArray2f& _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform2fv(location, _value.size(), (float*)_value.data()); }
    }
}

void ShaderProgram::setUniformf(const UniformLocation& _loc, const UniformArray3f& _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform3fv(location, _value.size(), (float*)_value.data()); }
    }
}

void ShaderProgram::setUniformi(const UniformLocation& _loc, const UniformTextureArray& _value) {
    if (!use()) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform1iv(location, _value.slots.size(), _value.slots.data()); }
    }
}

}
