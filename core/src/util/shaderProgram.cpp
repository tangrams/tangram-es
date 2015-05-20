#include "shaderProgram.h"
#include "scene/light.h"

GLuint ShaderProgram::s_activeGlProgram = 0;
int ShaderProgram::s_validGeneration = 0;

ShaderProgram::ShaderProgram() {

    m_glProgram = 0;
    m_glFragmentShader = 0;
    m_glVertexShader = 0;
    m_needsBuild = true;
    m_freeTextureUnit = 0;

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

    m_attribMap.clear();
    
}

void ShaderProgram::setSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc){
    m_fragmentShaderSource = std::string(_fragSrc);
    m_vertexShaderSource = std::string(_vertSrc);
    m_needsBuild = true;
}

void ShaderProgram::addSourceBlock(const std::string& _tagName, const std::string &_glslSource, bool _allowDuplicate){
    
    if (!_allowDuplicate) {
        for (auto& source : m_sourceBlocks[_tagName]) {
            if (_glslSource == source) {
                return;
            }
        }
    }
    
    m_sourceBlocks[_tagName].push_back("\n" + _glslSource);
    m_needsBuild = true;

    //  TODO:
    //          - add Global Blocks
}

void ShaderProgram::removeSourceBlock(const std::string& _tagName, const std::string& _glslSource){
    bool bChange = false;

    for (int i = m_sourceBlocks[_tagName].size()-1; i >= 0; i--) {
        if (_glslSource == m_sourceBlocks[_tagName][i]) {
            m_sourceBlocks[_tagName].erase(m_sourceBlocks[_tagName].begin()+i);
            bChange = true;
        }
    }

    m_needsBuild = bChange;
}

const GLint ShaderProgram::getAttribLocation(const std::string& _attribName) {

    // Get uniform location at this key, or create one valued at -2 if absent
    GLint& location = m_attribMap[_attribName].loc;

    // -2 means this is a new entry
    if (location == -2) {
        // Get the actual location from OpenGL
        location = glGetAttribLocation(m_glProgram, _attribName.c_str());
    }

    return location;

}

const GLint ShaderProgram::getUniformLocation(const std::string& _uniformName) {

    // Get uniform location at this key, or create one valued at -2 if absent
    GLint& location = m_uniformMap[_uniformName].loc;

    // -2 means this is a new entry
    if (location == -2) {
        // Get the actual location from OpenGL
        location = glGetUniformLocation(m_glProgram, _uniformName.c_str());
    }

    return location;

}

void ShaderProgram::use() {

    checkValidity();
    
    if (m_needsBuild) {
        build();
    }
    
    if (m_glProgram != 0 && m_glProgram != s_activeGlProgram) {
        glUseProgram(m_glProgram);
        s_activeGlProgram = m_glProgram;
    }

}

bool ShaderProgram::build() {
    
    m_needsBuild = false;
    m_generation = s_validGeneration;
    
    // Inject source blocks
    
    std::string vertSrc = m_vertexShaderSource;
    std::string fragSrc = m_fragmentShaderSource;
    applySourceBlocks(vertSrc, fragSrc);
    
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

    // New shaders linked successfully, so replace old shaders and program

    if (m_glProgram == s_activeGlProgram) {
        glUseProgram(0);
        s_activeGlProgram = 0;
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
    m_uniformMap.clear();

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
            logMsg("Error linking program:\n%s\n", &infoLog[0]);
        }
        glDeleteProgram(program);
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
            logMsg("Error compiling shader:\n%s\n", &infoLog[0]);
            //logMsg("\n%s\n", source); 
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;

}

void ShaderProgram::applySourceBlocks(std::string& _vertSrcOut, std::string& _fragSrcOut) {
    
    _vertSrcOut.insert(0, "#pragma tangram: defines\n");
    _fragSrcOut.insert(0, "#pragma tangram: defines\n");
    
    float depthDelta = 1.f / (1 << 16);
    _vertSrcOut.insert(0, "#define TANGRAM_DEPTH_DELTA "+std::to_string(depthDelta)+"\n");
    
    Light::assembleLights(m_sourceBlocks);
    
    for (auto& block : m_sourceBlocks) {
        
        std::string tag = "#pragma tangram: " + block.first;
        
        size_t vertSrcPos = _vertSrcOut.find(tag);
        size_t fragSrcPos = _fragSrcOut.find(tag);
        
        if (vertSrcPos != std::string::npos) {
            vertSrcPos += tag.length();
            for (auto& source : block.second) {
                _vertSrcOut.insert(vertSrcPos, source);
                vertSrcPos += source.length();
            }
        }
        if (fragSrcPos != std::string::npos) {
            fragSrcPos += tag.length();
            for (auto& source : block.second) {
                _fragSrcOut.insert(fragSrcPos, source);
                fragSrcPos += source.length();
            }
        }
    }
    
}

void ShaderProgram::checkValidity() {
    
    if (m_generation != s_validGeneration) {
        m_glFragmentShader = 0;
        m_glVertexShader = 0;
        m_glProgram = 0;
        m_needsBuild = true;
    }
    
}

void ShaderProgram::invalidateAllPrograms() {
    
    s_activeGlProgram = 0;
    ++s_validGeneration;
    
}

void ShaderProgram::setUniformi(const std::string& _name, int _value) {
    use();
    GLint location = getUniformLocation(_name);
    glUniform1i(location, _value);
}

void ShaderProgram::setUniformi(const std::string& _name, int _value0, int _value1) {
    use();
    GLint location = getUniformLocation(_name);
    glUniform2i(location, _value0, _value1);
}

void ShaderProgram::setUniformi(const std::string& _name, int _value0, int _value1, int _value2) {
    use();
    GLint location = getUniformLocation(_name);
    glUniform3i(location, _value0, _value1, _value2);
}

void ShaderProgram::setUniformi(const std::string& _name, int _value0, int _value1, int _value2, int _value3) {
    use();
    GLint location = getUniformLocation(_name);
    glUniform4i(location, _value0, _value1, _value2, _value3);
}

void ShaderProgram::setUniformf(const std::string& _name, float _value) {
    use();
    GLint location = getUniformLocation(_name);
    glUniform1f(location, _value);
}

void ShaderProgram::setUniformf(const std::string& _name, float _value0, float _value1) {
    use();
    GLint location = getUniformLocation(_name);
    glUniform2f(location, _value0, _value1);
}

void ShaderProgram::setUniformf(const std::string& _name, float _value0, float _value1, float _value2) {
    use();
    GLint location = getUniformLocation(_name);
    glUniform3f(location, _value0, _value1, _value2);
}

void ShaderProgram::setUniformf(const std::string& _name, float _value0, float _value1, float _value2, float _value3) {
    use();
    GLint location = getUniformLocation(_name);
    glUniform4f(location, _value0, _value1, _value2, _value3);
}

void ShaderProgram::setUniformMatrix2f(const std::string& _name, const float* _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    glUniformMatrix2fv(location, 1, _transpose, _value);
}

void ShaderProgram::setUniformMatrix3f(const std::string& _name, const float* _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    glUniformMatrix3fv(location, 1, _transpose, _value);
}

void ShaderProgram::setUniformMatrix4f(const std::string& _name, const float* _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    glUniformMatrix4fv(location, 1, _transpose, _value);
}
