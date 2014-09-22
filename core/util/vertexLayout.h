#pragma once

#include <vector>
#include <unordered_map>

#include "platform.h"
#include "util/shaderProgram.h"

class VertexLayout {
    
public:

    struct VertexAttrib {
        std::string name;
        GLint size;
        GLenum type;
        GLboolean normalized;
        GLvoid* offset; // Can be left as zero; value is overwritten in constructor of VertexLayout
    };

    VertexLayout(std::vector<VertexAttrib> _attribs);

    void enable(std::shared_ptr<ShaderProgram> _program);

    GLint getStride() const { return m_stride; };

private:

    static std::unordered_map<GLint, GLuint> s_enabledAttribs; // Map from attrib locations to bound shader program

    std::vector<VertexAttrib> m_attribs;
    GLint m_stride;

};
