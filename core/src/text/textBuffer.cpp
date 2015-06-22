#include "textBuffer.h"

TextBuffer::TextBuffer(FONScontext* _fsContext) : m_fsContext(_fsContext) {
    m_dirty = false;
}

void TextBuffer::init() {
    glfonsBufferCreate(m_fsContext, &m_fsBuffer);
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
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
}

fsuint TextBuffer::genTextID() {
    fsuint id;
    glfonsGenText(m_fsContext, 1, &id);
    return id;
}
    
bool TextBuffer::rasterize(const std::string& _text, fsuint _id) {
    int status = glfonsRasterize(m_fsContext, _id, _text.c_str());
    return status == GLFONS_VALID;
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
    glfonsBindBuffer(m_fsContext, 0);
}

glm::vec4 TextBuffer::getBBox(fsuint _textID) {
    glm::vec4 bbox;
    glfonsGetBBox(m_fsContext, _textID, &bbox.x, &bbox.y, &bbox.z, &bbox.w);
    return bbox;
}

bool TextBuffer::hasData() {
    auto mesh = getWeakMesh();
    if (mesh == nullptr) {
        return false;
    }
    return mesh->numVertices() > 0;
}

std::shared_ptr<VboMesh> TextBuffer::getWeakMesh() {
    return m_mesh.lock();
}
