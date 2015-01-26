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

void TextBuffer::bind() {
    m_bound = true;
    m_contextMutex->lock();
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
}

void TextBuffer::triggerTransformUpdate() {
    if(m_dirty && m_bound) {
        glfonsUpdateTransforms(m_fsContext, (void*) this);
        unbind();
    }
}

void TextBuffer::transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha) {

    if(!m_bound) {
        if(m_dirty) {
            logMsg("[WARNING][TextBuffer] Inconsistent buffer usage, shoud trigger transform updates first");
            return;
        }
        bind();
    } 

    glfonsTransform(m_fsContext, _textID, _x, _y, _rot, _alpha);

    m_dirty = true;
}

void TextBuffer::unbind() {
    glfonsBindBuffer(m_fsContext, 0);
    m_contextMutex->unlock();
    m_bound = false;
}
