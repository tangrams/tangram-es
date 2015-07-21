#include "vertexLayout.h"
#include "shaderProgram.h"
#include "platform.h"

std::unordered_map<GLint, GLuint> VertexLayout::s_enabledAttribs = std::unordered_map<GLint, GLuint>();

VertexLayout::VertexLayout(std::vector<VertexAttrib> _attribs) : m_attribs(_attribs) {

    m_stride = 0;

    for (auto& attrib : m_attribs) {

        // Set the offset of this vertex attribute: The stride at this point denotes the number
        // of bytes into the vertex by which this attribute is offset, but we must cast the number
        // as a void* to use with glVertexAttribPointer; We use reinterpret_cast to avoid warnings
        attrib.offset = m_stride;

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

        m_stride += byteSize;

        // TODO: Automatically add padding or warn if attributes are not byte-aligned

    }
}

VertexLayout::~VertexLayout() {

    m_attribs.clear();

}

size_t VertexLayout::getOffset(std::string _attribName) {
    
    for (auto& attrib : m_attribs) {
        if (attrib.name == _attribName) {
            return attrib.offset;
        }
    }

    logMsg("Error - No such attribute %s\n", _attribName.c_str());
    return 0;
}

void VertexLayout::enable(ShaderProgram& _program, size_t byteOffset, void* _ptr) {

    GLuint glProgram = _program.getGlProgram();

    // Enable all attributes for this layout
    for (auto& attrib : m_attribs) {

        GLint location = _program.getAttribLocation(attrib.name);

        if (location != -1) {
            // Track currently enabled attribs by the program to which they are bound
            if (s_enabledAttribs[location] != glProgram) {
                glEnableVertexAttribArray(location);
                s_enabledAttribs[location] = glProgram;
            }

            void* data = _ptr ? _ptr : ((unsigned char*) attrib.offset) + byteOffset;
            glVertexAttribPointer(location, attrib.size, attrib.type, attrib.normalized, m_stride, data);
        }
    }

    // Disable previously bound and now-unneeded attributes
    for (auto& locationProgramPair : s_enabledAttribs) {

        const GLint& location = locationProgramPair.first;
        GLuint& boundProgram = locationProgramPair.second;

        if (boundProgram != glProgram && boundProgram != 0) {
            glDisableVertexAttribArray(location);
            boundProgram = 0;
        }
    }
}
