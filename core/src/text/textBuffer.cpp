#include "textBuffer.h"
#include "fontContext.h"

#include "gl/texture.h"
#include "gl/vboMesh.h"

TextBuffer::TextBuffer(std::shared_ptr<FontContext> _fontContext, std::shared_ptr<VertexLayout> _vertexLayout)
    : TypedMesh<BufferVert>(_vertexLayout, GL_TRIANGLES, GL_DYNAMIC_DRAW) {

    m_dirtyTransform = false;
    m_fontContext = _fontContext;
    m_bound = false;
}

void TextBuffer::init() {
    m_fontContext->lock();
    glfonsBufferCreate(m_fontContext->getFontContext(), &m_fsBuffer);
    m_fontContext->unlock();
}

TextBuffer::~TextBuffer() {
    m_fontContext->lock();
    glfonsBufferDelete(m_fontContext->getFontContext(), m_fsBuffer);
    m_fontContext->unlock();
}

int TextBuffer::getVerticesSize() {
    bind();
    int size = glfonsVerticesSize(m_fontContext->getFontContext());
    unbind();
    return size;
}

fsuint TextBuffer::genTextID() {
    fsuint id;
    bind();
    glfonsGenText(m_fontContext->getFontContext(), 1, &id);
    unbind();
    return id;
}

bool TextBuffer::rasterize(const std::string& _text, fsuint _id) {
    bind();
    int status = glfonsRasterize(m_fontContext->getFontContext(), _id, _text.c_str());
    unbind();
    return status == GLFONS_VALID;
}

void TextBuffer::pushBuffer() {
    if (m_dirtyTransform) {
        bind();
        glfonsUpdateBuffer(m_fontContext->getFontContext(), this);
        unbind();
        m_dirtyTransform = false;
    }
}

void TextBuffer::transformID(fsuint _textID, const BufferVert::State& _state) {
    bind();
    glfonsTransform(m_fontContext->getFontContext(), _textID,
                    _state.screenPos.x, _state.screenPos.y,
                    _state.rotation, _state.alpha);
    unbind();
    m_dirtyTransform = true;
}

glm::vec4 TextBuffer::getBBox(fsuint _textID) {
    glm::vec4 bbox;
    bind();
    glfonsGetBBox(m_fontContext->getFontContext(), _textID, &bbox.x, &bbox.y, &bbox.z, &bbox.w);
    unbind();
    return bbox;
}

void TextBuffer::addBufferVerticesToMesh() {
    std::vector<BufferVert> vertices;
    int bufferSize = getVerticesSize();

    if (bufferSize == 0) {
        return;
    }

    vertices.resize(bufferSize);

    bind();
    bool res = glfonsVertices(m_fontContext->getFontContext(), reinterpret_cast<float*>(vertices.data()));
    unbind();

    if (res) {
        addVertices(std::move(vertices), {});
    }
}

void TextBuffer::bind() {
    if (!m_bound) {
        m_fontContext->lock();
        glfonsBindBuffer(m_fontContext->getFontContext(), m_fsBuffer);
        m_bound = true;
    }
}

void TextBuffer::unbind() {
    if (m_bound) {
        glfonsBindBuffer(m_fontContext->getFontContext(), 0);
        m_fontContext->unlock();
        m_bound = false;
    }
}
