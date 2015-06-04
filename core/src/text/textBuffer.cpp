#include "textBuffer.h"

TextBuffer::TextBuffer(FONScontext* _fsContext) : m_fsContext(_fsContext) {
    m_dirty = false;
    m_bound = false;
}

void TextBuffer::init(int _size) {
    glfonsBufferCreate(m_fsContext, _size, &m_fsBuffer);
}

TextBuffer::~TextBuffer() {
    glfonsBufferDelete(m_fsContext, m_fsBuffer);
}

int TextBuffer::getVerticesSize() {
    return glfonsVerticesSize(m_fsContext);
}

bool TextBuffer::getVertices(float* _vertices) {
    return glfonsVertices(m_fsContext, _vertices);
}

void TextBuffer::bind() {
    if (!m_bound) {
        m_bound = true;
        glfonsBindBuffer(m_fsContext, m_fsBuffer);
    }
}

fsuint TextBuffer::genTextID() {
    fsuint id;
    glfonsGenText(m_fsContext, 1, &id);
    return id;
}
    
void TextBuffer::rasterize(const std::string& _text, fsuint _id) {
    glfonsRasterize(m_fsContext, _id, _text.c_str());
}

void TextBuffer::pushBuffer() {
    if (m_dirty) {
        glfonsUpdateBuffer(m_fsContext);
        m_dirty = false;
    }
}

void TextBuffer::transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha) {
    glfonsTransform(m_fsContext, _textID, _x, _y, _rot, _alpha);

    m_dirty = true;
}

void TextBuffer::unbind() {
    if (m_bound) {
        glfonsBindBuffer(m_fsContext, 0);
        m_bound = false;
    }
}

glm::vec4 TextBuffer::getBBox(fsuint _textID) {
    glm::vec4 bbox;
    glfonsGetBBox(m_fsContext, _textID, &bbox.x, &bbox.y, &bbox.z, &bbox.w);
    return bbox;
}
