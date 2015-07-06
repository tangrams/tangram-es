#include "textBuffer.h"
#include "fontContext.h"

TextBuffer::TextBuffer(std::shared_ptr<FontContext> _fontContext, std::shared_ptr<VertexLayout> _vertexLayout)
    : TypedMesh<TextVert>(_vertexLayout, GL_TRIANGLES, GL_DYNAMIC_DRAW) {
        
    m_dirty = false;
    m_fsContext = _fontContext->getFontContext();
}

void TextBuffer::init() {
    glfonsBufferCreate(m_fsContext, &m_fsBuffer);
}

TextBuffer::~TextBuffer() {
    glfonsBufferDelete(m_fsContext, m_fsBuffer);
}

int TextBuffer::getVerticesSize() {
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
    return glfonsVerticesSize(m_fsContext);
}

bool TextBuffer::getVertices(float* _vertices) {
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
    return glfonsVertices(m_fsContext, _vertices);
}

fsuint TextBuffer::genTextID() {
    fsuint id;
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
    glfonsGenText(m_fsContext, 1, &id);
    return id;
}
    
bool TextBuffer::rasterize(const std::string& _text, fsuint _id) {
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
    int status = glfonsRasterize(m_fsContext, _id, _text.c_str());
    return status == GLFONS_VALID;
}

void TextBuffer::pushBuffer() {
    if (m_dirty) {
        glfonsBindBuffer(m_fsContext, m_fsBuffer);
        glfonsUpdateBuffer(m_fsContext);
        m_dirty = false;
    }
}

void TextBuffer::transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha) {
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
    glfonsTransform(m_fsContext, _textID, _x, _y, _rot, _alpha);

    m_dirty = true;
}

glm::vec4 TextBuffer::getBBox(fsuint _textID) {
    glm::vec4 bbox;
    glfonsBindBuffer(m_fsContext, m_fsBuffer);
    glfonsGetBBox(m_fsContext, _textID, &bbox.x, &bbox.y, &bbox.z, &bbox.w);
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
