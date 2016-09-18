#include "gl/vertexLayout.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/error.h"
#include "log.h"

namespace Tangram {

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

size_t VertexLayout::getOffset(std::string _attribName) {

    for (auto& attrib : m_attribs) {
        if (attrib.name == _attribName) {
            return attrib.offset;
        }
    }

    LOGE("No such attribute %s", _attribName.c_str());
    return 0;
}

void VertexLayout::enable(const fastmap<std::string, GLuint>& _locations, size_t _byteOffset) {

    for (auto& attrib : m_attribs) {
        auto it = _locations.find(attrib.name);

        if (it == _locations.end()) {
            continue;
        }

        GLint location = it->second;;

        if (location != -1) {
            void* offset = ((unsigned char*) attrib.offset) + _byteOffset;
            GL::enableVertexAttribArray(location);
            GL::vertexAttribPointer(location, attrib.size, attrib.type, attrib.normalized, m_stride, offset);
        }
    }

}

void VertexLayout::enable(RenderState& rs, ShaderProgram& _program, size_t _byteOffset, void* _ptr) {

    GLuint glProgram = _program.getGlProgram();

    // Enable all attributes for this layout
    for (auto& attrib : m_attribs) {

        GLint location = _program.getAttribLocation(attrib.name);

        if (location != -1) {
            auto& loc = rs.attributeBindings[location];
            // Track currently enabled attribs by the program to which they are bound
            if (loc != glProgram) {
                GL::enableVertexAttribArray(location);
                loc = glProgram;
            }

            void* data = (unsigned char*)_ptr + attrib.offset + _byteOffset;
            GL::vertexAttribPointer(location, attrib.size, attrib.type, attrib.normalized, m_stride, data);
        }
    }

    // Disable previously bound and now-unneeded attributes
    for (size_t i = 0; i < RenderState::MAX_ATTRIBUTES; ++i) {

        GLuint& boundProgram = rs.attributeBindings[i];

        if (boundProgram != glProgram && boundProgram != 0) {
            GL::disableVertexAttribArray(i);
            boundProgram = 0;
        }
    }
}

}
