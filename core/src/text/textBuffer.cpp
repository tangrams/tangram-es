#include "textBuffer.h"
#include "fontContext.h"

#include "style/textStyle.h"
#include "util/texture.h"
#include "util/vboMesh.h"

TextBatch::TextBatch(const TextStyle& _style)
    : m_fontContext(_style.m_labels->getFontContext()),
      m_mesh(std::make_shared<TypedMesh<BufferVert>>(_style.m_vertexLayout, GL_TRIANGLES, GL_DYNAMIC_DRAW)),
      m_style(_style)
{
        
    m_dirtyTransform = false;
    m_bound = false;
}

void TextBatch::init() {
    m_fontContext->lock();
    glfonsBufferCreate(m_fontContext->getFontContext(), &m_fsBuffer);
    m_fontContext->unlock();
}

TextBatch::~TextBatch() {
    m_fontContext->lock();
    glfonsBufferDelete(m_fontContext->getFontContext(), m_fsBuffer);
    m_fontContext->unlock();
}

int TextBatch::getVerticesSize() {
    bind();
    int size = glfonsVerticesSize(m_fontContext->getFontContext());
    unbind();
    return size;
}

fsuint TextBatch::genTextID() {
    fsuint id;
    bind();
    glfonsGenText(m_fontContext->getFontContext(), 1, &id);
    unbind();
    return id;
}
    
bool TextBatch::rasterize(const std::string& _text, fsuint _id) {
    bind();
    int status = glfonsRasterize(m_fontContext->getFontContext(), _id, _text.c_str());
    unbind();
    return status == GLFONS_VALID;
}

void TextBatch::pushBuffer() {
    if (m_dirtyTransform) {
        bind();
        glfonsUpdateBuffer(m_fontContext->getFontContext(), this);
        unbind();
        m_dirtyTransform = false;
    }
}

void TextBatch::transformID(fsuint _textID, const BufferVert::State& _state) {
    bind();
    glfonsTransform(m_fontContext->getFontContext(), _textID,
                    _state.screenPos.x, _state.screenPos.y,
                    _state.rotation, _state.alpha);
    unbind();
    m_dirtyTransform = true;
}

glm::vec4 TextBatch::getBBox(fsuint _textID) {
    glm::vec4 bbox;
    bind();
    glfonsGetBBox(m_fontContext->getFontContext(), _textID, &bbox.x, &bbox.y, &bbox.z, &bbox.w);
    unbind();
    return bbox;
}

void TextBatch::addBufferVerticesToMesh() {
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
        m_mesh->addVertices(std::move(vertices), {});
    }
}

void TextBatch::bind() {
    if (!m_bound) {
        m_fontContext->lock();
        glfonsBindBuffer(m_fontContext->getFontContext(), m_fsBuffer);
        m_bound = true;
    }
}

void TextBatch::unbind() {
    if (m_bound) {
        glfonsBindBuffer(m_fontContext->getFontContext(), 0);
        m_fontContext->unlock();
        m_bound = false;
    }
}

void TextBatch::draw(const View& _view) {
    m_mesh->draw(m_style.getShaderProgram());
};
