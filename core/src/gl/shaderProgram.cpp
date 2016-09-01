#include "shaderProgram.h"

#include "platform.h"
#include "scene/light.h"
#include "gl/disposer.h"
#include "gl/error.h"
#include "gl/renderState.h"
#include "glm/gtc/type_ptr.hpp"

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
    auto glFragmentShader = m_glFragmentShader;
    auto glVertexShader = m_glVertexShader;

    m_disposer([=](RenderState& rs) {
        if (rs.isValidGeneration(generation)) {
            if (glProgram != 0) {
                GL_CHECK(glDeleteProgram(glProgram));
            }

            if (glFragmentShader != 0) {
                GL_CHECK(glDeleteShader(glFragmentShader));
            }

            if (glVertexShader != 0) {
                GL_CHECK(glDeleteShader(glVertexShader));
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

    m_sourceBlocks[_tagName].push_back(_glslSource);
    m_needsBuild = true;

    //  TODO:
    //          - add Global Blocks
}

GLint ShaderProgram::getAttribLocation(const std::string& _attribName) {

    auto it = m_attribMap.find(_attribName);

    if (it == m_attribMap.end()) {
        // If this is a new entry, get the actual location from OpenGL.
        GLint location = glGetAttribLocation(m_glProgram, _attribName.c_str());
        GL_CHECK();
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
    _uniform.location = glGetUniformLocation(m_glProgram, _uniform.name.c_str());
    GL_CHECK();

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

    m_needsBuild = false;
    m_generation = rs.generation();

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
        GL_CHECK(glDeleteShader(vertexShader));
        return false;
    }

    // Try to link shaders into a program, releasing resources and quiting on failure

    GLint program = makeLinkedShaderProgram(fragmentShader, vertexShader);

    if (program == 0) {
        GL_CHECK(glDeleteShader(vertexShader));
        GL_CHECK(glDeleteShader(fragmentShader));
        return false;
    }

    // Delete handles for old shaders and program; values of 0 are silently ignored

    GL_CHECK(glDeleteShader(m_glFragmentShader));
    GL_CHECK(glDeleteShader(m_glVertexShader));
    GL_CHECK(glDeleteProgram(m_glProgram));

    m_glFragmentShader = fragmentShader;
    m_glVertexShader = vertexShader;
    m_glProgram = program;

    // Clear any cached shader locations

    m_attribMap.clear();
    m_disposer = Disposer(rs);

    return true;
}

GLuint ShaderProgram::makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader) {

    GLuint program = glCreateProgram();
    GL_CHECK();

    GL_CHECK(glAttachShader(program, _fragShader));
    GL_CHECK(glAttachShader(program, _vertShader));
    GL_CHECK(glLinkProgram(program));

    GLint isLinked;
    GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &isLinked));

    if (isLinked == GL_FALSE) {
        GLint infoLength = 0;
        GL_CHECK(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength));

        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            GL_CHECK(glGetProgramInfoLog(program, infoLength, NULL, &infoLog[0]));
            LOGE("linking program:\n%s", &infoLog[0]);
        }

        GL_CHECK(glDeleteProgram(program));
        m_invalidShaderSource = true;
        return 0;
    }

    return program;
}

GLuint ShaderProgram::makeCompiledShader(const std::string& _src, GLenum _type) {

    GLuint shader = glCreateShader(_type);
    GL_CHECK();

    const GLchar* source = (const GLchar*) _src.c_str();
    GL_CHECK(glShaderSource(shader, 1, &source, NULL));
    GL_CHECK(glCompileShader(shader));

    GLint isCompiled;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled));

    if (isCompiled == GL_FALSE) {
        GLint infoLength = 0;
        GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength));

        if (infoLength > 1) {
            std::string infoLog;
            infoLog.resize(infoLength);

            GL_CHECK(glGetShaderInfoLog(shader, infoLength, NULL, static_cast<GLchar*>(&infoLog[0])));
            LOGE("Shader compilation failed %s", m_description.c_str());

            std::stringstream ss(source);
            std::string item;
            std::vector<std::string> sourceLines;
            while (std::getline(ss, item, '\n')) { sourceLines.push_back(item); }

            // Print errors with context
            std::string line;
            ss.str(infoLog);
            while (std::getline(ss, line)) {
                if (line.length() < 2) { continue; }

                int lineNum = 0;
                sscanf(line.c_str(), "%*d(%d)", &lineNum);
                LOGE("\nError on line %d: %s\n", lineNum, line.c_str());

                for (int i = std::max(0, lineNum-5); i < lineNum+5; i++) {
                    if (size_t(i) >= sourceLines.size()) { break; }
                    LOGE("%d: %s\n", i, sourceLines[i].c_str());
                }
            }

            // Print full source with line numbers
            LOGD("\n\n");
            for (size_t i = 0; i < sourceLines.size(); i++) {
                LOGD("%d: %s", i, sourceLines[i].c_str());
            }
        }

        GL_CHECK(glDeleteShader(shader));
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

void ShaderProgram::checkValidity(RenderState& rs) {

    if (!rs.isValidGeneration(m_generation)) {
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

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, int _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniform1i(location, _value)); }
    }
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec2(_value0, _value1));
        if (!cached) { GL_CHECK(glUniform2i(location, _value0, _value1)); }
    }
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1, int _value2) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec3(_value0, _value1, _value2));
        if (!cached) { GL_CHECK(glUniform3i(location, _value0, _value1, _value2)); }
    }
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1, int _value2, int _value3) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec4(_value0, _value1, _value2, _value3));
        if (!cached) { GL_CHECK(glUniform4i(location, _value0, _value1, _value2, _value3)); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, float _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniform1f(location, _value)); }
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
        if (!cached) { GL_CHECK(glUniform2f(location, _value.x, _value.y)); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const glm::vec3& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniform3f(location, _value.x, _value.y, _value.z)); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const glm::vec4& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniform4f(location, _value.x, _value.y, _value.z, _value.w)); }
    }
}

void ShaderProgram::setUniformMatrix2f(RenderState& rs, const UniformLocation& _loc, const glm::mat2& _value, bool _transpose) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniformMatrix2fv(location, 1, _transpose, glm::value_ptr(_value))); }
    }
}

void ShaderProgram::setUniformMatrix3f(RenderState& rs, const UniformLocation& _loc, const glm::mat3& _value, bool _transpose) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniformMatrix3fv(location, 1, _transpose, glm::value_ptr(_value))); }
    }
}

void ShaderProgram::setUniformMatrix4f(RenderState& rs, const UniformLocation& _loc, const glm::mat4& _value, bool _transpose) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = !_transpose && getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniformMatrix4fv(location, 1, _transpose, glm::value_ptr(_value))); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray1f& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniform1fv(location, _value.size(), _value.data())); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray2f& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform2fv(location, _value.size(), (float*)_value.data()); }
    }
}

void ShaderProgram::setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray3f& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { glUniform3fv(location, _value.size(), (float*)_value.data()); }
    }
}

void ShaderProgram::setUniformi(RenderState& rs, const UniformLocation& _loc, const UniformTextureArray& _value) {
    if (!use(rs)) { return; }
    GLint location = getUniformLocation(_loc);
    if (location >= 0) {
        bool cached = getFromCache(location, _value);
        if (!cached) { GL_CHECK(glUniform1iv(location, _value.slots.size(), _value.slots.data())); }
    }
}

}
