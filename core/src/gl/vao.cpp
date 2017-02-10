#include "gl/vao.h"
#include "gl/error.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/vertexLayout.h"
#include <unordered_map>

namespace Tangram {

void Vao::initialize(RenderState& rs, ShaderProgram& _program, const std::vector<std::pair<uint32_t, uint32_t>>& _vertexOffsets,
               VertexLayout& _layout, GLuint _vertexBuffer, GLuint _indexBuffer) {

    m_glVAOs.resize(_vertexOffsets.size());

    GL::genVertexArrays(m_glVAOs.size(), m_glVAOs.data());

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
        GL::bindVertexArray(m_glVAOs[i]);

        rs.vertexBufferUnset(_vertexBuffer);
        rs.vertexBuffer(_vertexBuffer);

        if (_indexBuffer != 0) {
            rs.indexBufferUnset(_indexBuffer);
            rs.indexBuffer(_indexBuffer);
        }

        // Enable vertex layout on the specified locations
        _layout.enable(locations, vertexOffset * _layout.getStride());

        vertexOffset += nVerts;
    }

}

bool Vao::isInitialized() {
    return !m_glVAOs.empty();
}

void Vao::bind(unsigned int _index) {
    if (_index < m_glVAOs.size()) {
        GL::bindVertexArray(m_glVAOs[_index]);
    }
}

void Vao::unbind() {
    GL::bindVertexArray(0);
}

void Vao::dispose() {
    if (!m_glVAOs.empty()) {
        GL::deleteVertexArrays(m_glVAOs.size(), m_glVAOs.data());
        m_glVAOs.clear();
    }
}

}

