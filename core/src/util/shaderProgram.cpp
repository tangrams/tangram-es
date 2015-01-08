#include "shaderProgram.h"
#include "util/stringsOp.h"

GLuint ShaderProgram::s_activeGlProgram = 0;
std::unordered_set<ShaderProgram*> ShaderProgram::s_managedPrograms;

ShaderProgram::ShaderProgram() {

    m_glProgram = 0;
    m_glFragmentShader = 0;
    m_glVertexShader = 0;
    m_needsBuild = true;
    
    addManagedProgram(this);

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
    
    removeManagedProgram(this);
}

void ShaderProgram::setSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc){
    m_fragmentShaderSource = std::string(_fragSrc);
    m_vertexShaderSource = std::string(_vertSrc);
    m_needsBuild = true;
}

void ShaderProgram::addSourceBlock(const std::string& _tagName, const std::string &_glslSource){
    m_sourceBlocks[_tagName].push_back(_glslSource+"\n");
    m_needsBuild = true;
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
    
    // Inject source blocks
    
    std::string vertSrc = m_vertexShaderSource;
    std::string fragSrc = m_fragmentShaderSource;
    
    for (auto& block : m_sourceBlocks) {

        std::string blockSum = "\n";

        for (auto& source : block.second) {
            blockSum += source + "\n";
        }
        
        std::string tag = "#pragma tangram: " + block.first;
        
        int tagPos = fragSrc.find(tag);
        
        if (tagPos != std::string::npos) {
            fragSrc.insert(tagPos + tag.length(), blockSum);
        }
        
        tagPos = vertSrc.find(tag);
        
        if (tagPos != std::string::npos) {
            vertSrc.insert(tagPos + tag.length(), blockSum);
        }

    }
    
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
    
    logMsg("Final compiled vertex shader: \n%s\n", vertSrc.c_str());
    logMsg("Final compiled fragment shader: \n%s\n", fragSrc.c_str());

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
            logMsg("\n> Error ----------------------->>\n%s\n",getLineNumberString(_src).c_str());
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;

}

void ShaderProgram::addManagedProgram(ShaderProgram* _program) {
    
    s_managedPrograms.insert(_program);
    
}

void ShaderProgram::removeManagedProgram(ShaderProgram* _program) {
    
    s_managedPrograms.erase(_program);
    
}

void ShaderProgram::invalidateAllPrograms() {
    
    s_activeGlProgram = 0;
    
    for (auto prog : s_managedPrograms) {
        
        // Set all OpenGL handles to invalidated values
        prog->m_glFragmentShader = 0;
        prog->m_glVertexShader = 0;
        prog->m_glProgram = 0;

        // Generate new handles by recompiling from saved source strings
        prog->build();
        
    }
    
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

void ShaderProgram::setUniformMatrix2f(const std::string& _name, float* _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    glUniformMatrix2fv(location, 1, _transpose, _value);
}

void ShaderProgram::setUniformMatrix3f(const std::string& _name, float* _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    glUniformMatrix3fv(location, 1, _transpose, _value);
}

void ShaderProgram::setUniformMatrix4f(const std::string& _name, float* _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    glUniformMatrix4fv(location, 1, _transpose, _value);
}