#include "textBuffer.h"

std::unique_ptr<std::mutex> TextBuffer::s_contextMutex = std_patch::make_unique<std::mutex>();

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
    s_contextMutex->lock();
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
}

fsuint TextBuffer::genTextID() {    
    if (!m_bound) {
        bind();
    }

    fsuint id;
    glfonsGenText(m_fsContext, 1, &id);
    
    return id;
}
    
void TextBuffer::rasterize(const std::string& _text, fsuint _id) {
    if (!m_bound) {
        bind();
    }

    glfonsRasterize(m_fsContext, _id, _text.c_str(), FONS_EFFECT_NONE /* TODO : update signature, remove effect */);
}

void TextBuffer::triggerTransformUpdate() {
    if (m_dirty && m_bound) {
        glfonsUpdateTransforms(m_fsContext, (void*) this);
        unbind();
    }
}

void TextBuffer::transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha) {
    if (!m_bound) {
        if (m_dirty) {
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
    s_contextMutex->unlock();
    m_bound = false;
}
