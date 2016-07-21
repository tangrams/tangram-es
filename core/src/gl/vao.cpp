#include "vao.h"
#include "renderState.h"
#include "shaderProgram.h"
#include "vertexLayout.h"
#include <unordered_map>

namespace Tangram {

Vao::Vao() {
    m_glVAOs = nullptr;
    m_glnVAOs = 0;
}

Vao::~Vao() {
    if (m_glVAOs) {
        GL_CHECK(glDeleteVertexArrays(m_glnVAOs, m_glVAOs));
        delete[] m_glVAOs;
    }
}

void Vao::init(RenderState& rs, ShaderProgram& _program, const std::vector<std::pair<uint32_t, uint32_t>>& _vertexOffsets,
               VertexLayout& _layout, GLuint _vertexBuffer, GLuint _indexBuffer) {

    m_glnVAOs = _vertexOffsets.size();
    m_glVAOs = new GLuint[m_glnVAOs];

    GL_CHECK(glGenVertexArrays(m_glnVAOs, m_glVAOs));

    fastmap<std::string, GLuint> locations;

    // FIXME (use a bindAttrib instead of getLocation) to make those locations shader independent
    for (auto& attrib : _layout.getAttribs()) {
        GLint location = _program.getAttribLocation(attrib.name);
        locations[attrib.name] = location;
    }

    int vertexOffset = 0;
    for (size_t i = 0; i < _vertexOffsets.size(); ++i) {
        auto vertexIndexOffset = _vertexOffsets[i];
        int nVerts = vertexIndexOffset.second;
        GL_CHECK(glBindVertexArray(m_glVAOs[i]));

        rs.vertexBuffer.init(_vertexBuffer, true);

        if (_indexBuffer != 0) {
            rs.indexBuffer.init(_indexBuffer, true);
        }

        // Enable vertex layout on the specified locations
        _layout.enable(locations, vertexOffset * _layout.getStride());

        vertexOffset += nVerts;
    }

}

void Vao::bind(unsigned int _index) {
    if (_index < m_glnVAOs) {
        GL_CHECK(glBindVertexArray(m_glVAOs[_index]));
    }
}

void Vao::unbind() {
    GL_CHECK(glBindVertexArray(0));
}

void Vao::discard() {
    delete[] m_glVAOs;
    m_glVAOs = nullptr;
}

}

