#include "textBuffer.h"
#include "fontContext.h"

#include "util/texture.h"
#include "util/vboMesh.h"

TextBuffer::TextBuffer(std::shared_ptr<FontContext> _fontContext, std::shared_ptr<VertexLayout> _vertexLayout)
    : TypedMesh<TextVert>(_vertexLayout, GL_TRIANGLES, GL_DYNAMIC_DRAW) {
        
    m_dirty = false;
    m_fontContext = _fontContext;
    m_bound = false;
}

void TextBuffer::init() {
    m_fontContext->lock();
    glfonsBufferCreate(m_fontContext->getFontContext(), &m_fsBuffer);
    m_fontContext->unlock();
}

TextBuffer::~TextBuffer() {
    glfonsBufferDelete(m_fontContext->getFontContext(), m_fsBuffer);
}

int TextBuffer::getVerticesSize() {
    bind();
    int size = glfonsVerticesSize(m_fontContext->getFontContext());
    unbind();
    return size;
}

bool TextBuffer::getVertices(float* _vertices) {
    bind();
    bool res = glfonsVertices(m_fontContext->getFontContext(), _vertices);
    unbind();
    return res;
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
    if (m_dirty) {
        bind();
        glfonsUpdateBuffer(m_fontContext->getFontContext());
        unbind();
        m_dirty = false;
    }
}

void TextBuffer::transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha) {
    bind();
    glfonsTransform(m_fontContext->getFontContext(), _textID, _x, _y, _rot, _alpha);
    unbind();
    m_dirty = true;
}

glm::vec4 TextBuffer::getBBox(fsuint _textID) {
    glm::vec4 bbox;
    bind();
    glfonsGetBBox(m_fontContext->getFontContext(), _textID, &bbox.x, &bbox.y, &bbox.z, &bbox.w);
    unbind();
    return bbox;
}

void TextBuffer::finish() {
    std::vector<TextVert> vertices;
    int bufferSize = getVerticesSize();
    
    if (bufferSize == 0) {
        return;
    }
    
    vertices.resize(bufferSize);
    
    if (getVertices(reinterpret_cast<float*>(vertices.data()))) {
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
