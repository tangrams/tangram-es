#pragma once

#include "gl.h"

#include "util/fastmap.h"

#include <vector>
#include <memory>
#include <string>

namespace Tangram {

class RenderState;
class ShaderProgram;

class VertexLayout {

public:

    struct VertexAttrib {
        std::string name;
        GLint size;
        GLenum type;
        GLboolean normalized;
        size_t offset; // Can be left as zero; value is overwritten in constructor of VertexLayout
    };

    VertexLayout(std::vector<VertexAttrib> _attribs);

    void enable(RenderState& rs, ShaderProgram& _program, size_t _byteOffset, void* _ptr = nullptr);

    void enable(const fastmap<std::string, GLuint>& _locations, size_t _bytOffset);

    GLint getStride() const { return m_stride; };

    const std::vector<VertexAttrib> getAttribs() const { return m_attribs; }

    size_t getOffset(std::string _attribName);

private:

    std::vector<VertexAttrib> m_attribs;
    GLint m_stride;

};

}
