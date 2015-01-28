#include "textBuffer.h"

TextBuffer::TextBuffer(FONScontext* _fsContext) : TextBuffer(_fsContext, 2) {}

TextBuffer::TextBuffer(FONScontext* _fsContext, int _size) : m_fsContext(_fsContext) {
    m_dirty = false;
    m_bound = false;
    glfonsBufferCreate(_fsContext, _size, &m_fsBuffer);
}

TextBuffer::~TextBuffer() {
    glfonsBufferDelete(m_fsContext, m_fsBuffer);
    m_transform->destroy();
}

void TextBuffer::setTextureTransform(std::unique_ptr<Texture> _texture) {
    m_transform = std::move(_texture);
}

const std::unique_ptr<Texture>& TextBuffer::getTextureTransform() const {
    return m_transform;
}

void TextBuffer::getVertices(std::vector<float>* _vertices, int* _nVerts) {
    glfonsVertices(m_fsContext, _vertices, _nVerts);
}

void TextBuffer::bind() {
    if (!m_bound) {
        m_bound = true;
        glfonsBindBuffer(m_fsContext, m_fsBuffer);
    }
}

fsuint TextBuffer::genTextID() {    
    bind();
    fsuint id;
    glfonsGenText(m_fsContext, 1, &id);
    unbind();
    
    return id;
}
    
void TextBuffer::rasterize(const std::string& _text, fsuint _id) {
    bind();
    glfonsRasterize(m_fsContext, _id, _text.c_str(), FONS_EFFECT_NONE /* TODO : update signature, remove effect */);
    unbind();
}

void TextBuffer::triggerTransformUpdate() {
    if (m_dirty) {
        bind();
        glfonsUpdateTransforms(m_fsContext, (void*) this);
        unbind();
    }
}

void TextBuffer::transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha) {
    bind(); 
    glfonsTransform(m_fsContext, _textID, _x, _y, _rot, _alpha);
    m_dirty = true;
    unbind();
}

void TextBuffer::unbind() {
    glfonsBindBuffer(m_fsContext, 0);
    m_bound = false;
}
