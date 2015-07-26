#include "textBuffer.h"
#include "fontContext.h"

#include "gl/texture.h"
#include "gl/vboMesh.h"
#include "labels/labels.h"

namespace Tangram {

TextBuffer::TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout)
    : LabelMesh(_vertexLayout, GL_TRIANGLES) {

    m_dirtyTransform = false;
    m_bufferPosition = 0;

    m_fontContext = Labels::GetInstance()->getFontContext();
}

void TextBuffer::init(fsuint _fontID, float _size, float _blurSpread) {
    m_fontID = _fontID;
    m_fontSize = _size;
    m_fontBlurSpread = _blurSpread;

    m_fontContext->lock();
    glfonsBufferCreate(m_fontContext->getFontContext(), &m_fsBuffer);
    m_fontContext->unlock();
}

TextBuffer::~TextBuffer() {
    m_fontContext->lock();
    glfonsBufferDelete(m_fontContext->getFontContext(), m_fsBuffer);
    m_fontContext->unlock();
}

int TextBuffer::rasterize(const std::string& _text, glm::vec2& _size, size_t& _bufferOffset) {
    int numGlyphs = 0;

    m_fontContext->lock();

    auto ctx = m_fontContext->getFontContext();
    glfonsBindBuffer(ctx, m_fsBuffer);

    fsuint textID;
    glfonsGenText(ctx, 1, &textID);

    fonsSetSize(ctx, m_fontSize);
    fonsSetFont(ctx, m_fontID);

    if (m_fontBlurSpread > 0){
        fonsSetBlur(ctx, m_fontBlurSpread);
        fonsSetBlurType(ctx, FONS_EFFECT_DISTANCE_FIELD);
    } else {
        fonsSetBlurType(ctx, FONS_EFFECT_NONE);
    }

    int status = glfonsRasterize(ctx, textID, _text.c_str());
    if (status == GLFONS_VALID) {
        numGlyphs = glfonsGetGlyphCount(ctx, textID);
        _bufferOffset = m_bufferPosition;

        m_bufferPosition += m_vertexLayout->getStride() * numGlyphs * 6;

        glm::vec4 bbox;
        glfonsGetBBox(ctx, textID, &bbox.x, &bbox.y, &bbox.z, &bbox.w);

        _size.x = std::abs(bbox.z - bbox.x);
        _size.y = std::abs(bbox.w - bbox.y);
    }

    glfonsBindBuffer(ctx, 0);
    m_fontContext->unlock();

    return numGlyphs;
}

void TextBuffer::addBufferVerticesToMesh() {
    std::vector<Label::Vertex> vertices;

    m_fontContext->lock();
    auto ctx = m_fontContext->getFontContext();

    glfonsBindBuffer(ctx, m_fsBuffer);
    int bufferSize = glfonsVerticesSize(ctx);

    if (bufferSize == 0) {
        glfonsBindBuffer(ctx, 0);
        glfonsBufferDelete(ctx, m_fsBuffer);
        m_fsBuffer = 0;

        m_fontContext->unlock();
        return;
    }

    vertices.resize(bufferSize);

    bool res = glfonsVertices(ctx, reinterpret_cast<float*>(vertices.data()));

    if (res) {
        addVertices(std::move(vertices), {});
    }

    glfonsBindBuffer(ctx, 0);
    glfonsBufferDelete(ctx, m_fsBuffer);
    m_fsBuffer = 0;

    m_fontContext->unlock();
}

}
