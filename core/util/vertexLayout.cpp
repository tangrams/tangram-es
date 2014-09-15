#include "vertexLayout.h"

VertexLayout::VertexLayout(std::vector<VertexAttrib> _attribs) {

    m_stride = 0; 

    m_attribs = _attribs;

    for (auto& attrib : m_attribs) {

        GLint byteSize = attrib.size;

        switch (attrib.type) {
            case GL_FLOAT:
            case GL_INT:
            case GL_UNSIGNED_INT:
                byteSize *= 4; // 4 bytes for floats, ints, and uints
                break;
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
                byteSize *= 2; // 2 bytes for shorts and ushorts
                break;
        }

        attrib.offset = m_stride;

        m_stride += byteSize;

    }
}

void VertexLayout::enable(ShaderProgram _program) {

    GLint glProgram = _program.getGlProgram();

    // Enable all attributes for this layout
    for (auto& attrib : m_attribs) {

        GLint location = _program.getAttribLocation(attrib.name);

        if (location != -1) {
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, attrib.size, attrib.type, attrib.normalized, m_stride, attrib.offset);
            s_enabledAttribs[location] = glProgram; // Track currently enable attribs by the program they are bound to
        }

    }

    // Disable previously bound and now-unneeded attributes
    for (auto locationProgramPair : s_enabledAttribs) {

        GLint& location = locationProgramPair.first;
        GLint& boundProgram = locationProgramPair.second;

        if (boundProgram != glProgram && boundProgram != 0) {
            glDisableVertexAttribArray(location);
            boundProgram = 0; 
        }

    }

}