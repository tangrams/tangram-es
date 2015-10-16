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
        glDeleteVertexArrays(m_glnVAOs, m_glVAOs);
        delete[] m_glVAOs;
    }
}

void Vao::init(ShaderProgram& _program, const std::vector<std::pair<uint32_t, uint32_t>>& _vertexOffsets,
               VertexLayout& _layout, GLuint _vertexBuffer, GLuint _indexBuffer) {

    m_glnVAOs = _vertexOffsets.size();
    m_glVAOs = new GLuint[m_glnVAOs];

    glGenVertexArrays(m_glnVAOs, m_glVAOs);

    std::unordered_map<std::string, GLuint> locations;

    // FIXME (use a bindAttrib instead of getLocation) to make those locations shader independent
    for (auto& attrib : _layout.getAttribs()) {
        GLint location = _program.getAttribLocation(attrib.name);
        locations[attrib.name] = location;
    }

    int vertexOffset = 0;
    for (int i = 0; i < _vertexOffsets.size(); ++i) {
        auto vertexIndexOffset = _vertexOffsets[i];
        int nVerts = vertexIndexOffset.second;
        glBindVertexArray(m_glVAOs[i]);

        RenderState::vertexBuffer.init(_vertexBuffer, true);

        if (_indexBuffer != -1) {
            RenderState::indexBuffer.init(_indexBuffer, true);
        }

        // Enable vertex layout on the specified locations
        _layout.enable(locations, vertexOffset * _layout.getStride());

        vertexOffset += nVerts;
    }

}

void Vao::bind(unsigned int _index) {
    if (_index < m_glnVAOs) {
        glBindVertexArray(m_glVAOs[_index]);
    }
}

void Vao::unbind() {
    glBindVertexArray(0);
}

}

