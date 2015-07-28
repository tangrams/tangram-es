#include "textBuffer.h"
#include "labels/textLabel.h"

#include "gl/texture.h"
#include "gl/vboMesh.h"

namespace Tangram {

TextBuffer::TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout)
    : LabelMesh(_vertexLayout, GL_TRIANGLES) {

    m_dirtyTransform = false;
    addVertices({}, {});
}

void TextBuffer::init(uint32_t _fontID, float _size, float _blurSpread) {
    m_fontID = _fontID;
    m_fontSize = _size;
    m_fontBlurSpread = _blurSpread;
}

TextBuffer::~TextBuffer() {
}

bool TextBuffer::addLabel(const std::string& _text, Label::Transform _transform, Label::Type _type) {

    auto fontContext = FontContext::GetInstance();

    fontContext->lock();

    auto& quads = fontContext->rasterize(_text, m_fontID, m_fontSize, m_fontBlurSpread);
    size_t numGlyphs = quads.size();

    if (numGlyphs == 0) {
        fontContext->unlock();
        return false;
    }

    auto& verts = vertices[0];

    // byte offset of added vertices
    size_t bufferPosition = m_vertexLayout->getStride() * verts.size();

    verts.reserve(verts.size() + numGlyphs * 6);

    float inf = std::numeric_limits<float>::infinity();
    float x0 = inf, x1 = -inf, y0 = inf, y1 = -inf;

    for (auto& q : quads) {
        x0 = std::min(x0, std::min(q.x0, q.x1));
        x1 = std::max(x1, std::max(q.x0, q.x1));
        y0 = std::min(y0, std::min(q.y0, q.y1));
        y1 = std::max(y1, std::max(q.y0, q.y1));

        verts.push_back({{q.x0, q.y0}, {q.s0, q.t0}});
        verts.push_back({{q.x1, q.y1}, {q.s1, q.t1}});
        verts.push_back({{q.x1, q.y0}, {q.s1, q.t0}});

        verts.push_back({{q.x0, q.y0}, {q.s0, q.t0}});
        verts.push_back({{q.x0, q.y1}, {q.s0, q.t1}});
        verts.push_back({{q.x1, q.y1}, {q.s1, q.t1}});
    }

    fontContext->unlock();

    glm::vec2 size((x1 - x0), (y1 - y0));

    m_labels.emplace_back(new TextLabel(_text, _transform, _type,
                                        numGlyphs, size,
                                        *this, bufferPosition));

    // TODO: change this in TypeMesh::adVertices()
    m_nVertices = verts.size();

    return true;
}

}
