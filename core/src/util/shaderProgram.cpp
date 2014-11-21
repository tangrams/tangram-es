#include "shaderProgram.h"

GLint ShaderProgram::s_activeGlProgram = 0;

ShaderProgram::ShaderProgram() {

    m_glProgram = 0;
    m_glFragmentShader = 0;
    m_glVertexShader = 0;

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

    // Zero means this is a new entry
    if (location == -2) {
        // Get the actual location from OpenGL
        location = glGetUniformLocation(m_glProgram, _uniformName.c_str());
    }

    return location;

}

void ShaderProgram::use() const {

    if (m_glProgram != 0 && m_glProgram != s_activeGlProgram) {
        glUseProgram(m_glProgram);
        s_activeGlProgram = m_glProgram;
    }
}

//  THIS SHOULD BE REPLACER FOR A REAL PARSER/INJECTOR pluss the globas and transform commands
//
std::vector<std::string> splitString(const std::string &_source, const std::string &_delimiter = "", bool _ignoreEmpty = false) {
    std::vector<std::string> result;
    if (_delimiter.empty()) {
        result.push_back(_source);
        return result;
    }
    std::string::const_iterator substart = _source.begin(), subend;
    while (true) {
        subend = search(substart, _source.end(), _delimiter.begin(), _delimiter.end());
        std::string sub(substart, subend);
        
        if (!_ignoreEmpty || !sub.empty()) {
            result.push_back(sub);
        }
        if (subend == _source.end()) {
            break;
        }
        substart = subend + _delimiter.size();
    }
    return result;
}

std::string dummyParser(const std::string &_src){
    std::vector<std::string> lines = splitString(_src, "\n");
    
    std::string rta;
    
    for (auto &line: lines) {
        if (line == "#pragma tangram: lighting") {
            rta += stringFromResource("lights.glsl") + "\n";
        } else {
            rta += line + "\n";
        }
    }
    
    return rta;
}

bool ShaderProgram::buildFromSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc) {

    //  TODO:
    //          - This is a hardcode injection
    //          - Here will happen the real shader injection
    
    std::string glslHeader = "#ifdef GL_ES\nprecision mediump float;\n#endif\n\n";
    std::string vertSrc = glslHeader + dummyParser(_vertSrc);
    std::string fragSrc = glslHeader + dummyParser(_fragSrc);
    
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

    glDeleteShader(m_glFragmentShader);
    glDeleteShader(m_glVertexShader);
    glDeleteProgram(m_glProgram);

    m_glFragmentShader = fragmentShader;
    m_glVertexShader = vertexShader;
    m_glProgram = program;

    // Make copies of the shader source code inputs, for this program to keep

    m_fragmentShaderSource = std::string(_fragSrc);
    m_vertexShaderSource = std::string(_vertSrc);

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
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;

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