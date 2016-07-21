#include "renderState.h"

#include "platform.h"
#include "vertexLayout.h"
#include "gl/hardware.h"

namespace Tangram {

static size_t max = std::numeric_limits<size_t>::max();

GLuint RenderState::getTextureUnit(GLuint _unit) {
    return GL_TEXTURE0 + _unit;
}

void RenderState::bindVertexBuffer(GLuint _id) {
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _id));
}

void RenderState::bindIndexBuffer(GLuint _id) {
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id));
}

void RenderState::bindTexture(GLenum _target, GLuint _textureId) {
    GL_CHECK(glBindTexture(_target, _textureId));
}

void RenderState::activeTextureUnit(GLuint _unit) {
    // current texture unit is changing, invalidate current texture binding:
    // FIXME: texture.init(GL_TEXTURE_2D, max, false);
    GL_CHECK(glActiveTexture(getTextureUnit(_unit)));
}

void RenderState::invalidate() {
    m_textureUnit = -1;
    VertexLayout::clearCache();

    blending.init(GL_FALSE);
    blendingFunc.init(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    culling.init(GL_TRUE);
    cullFace.init(GL_BACK);
    frontFace.init(GL_CCW);
    depthTest.init(GL_TRUE);
    depthWrite.init(GL_TRUE);

    GL_CHECK(glDisable(GL_STENCIL_TEST));
    GL_CHECK(glDepthFunc(GL_LESS));
    GL_CHECK(glClearDepthf(1.0));
    GL_CHECK(glDepthRangef(0.0, 1.0));

    clearColor.init(0.0, 0.0, 0.0, 0.0);
    shaderProgram.init(max, false);
    vertexBuffer.init(max, false);
    indexBuffer.init(max, false);
    texture.init(GL_TEXTURE_2D, max, false);
    texture.init(GL_TEXTURE_CUBE_MAP, max, false);
    textureUnit.init(max, false);
}

void RenderState::increaseGeneration() {
    m_validGeneration++;
}

bool RenderState::isValidGeneration(int _generation) {
    return _generation == m_validGeneration;
}

int RenderState::generation() {
    return m_validGeneration;
}

int RenderState::nextAvailableTextureUnit() {
    if (m_textureUnit >= Hardware::maxCombinedTextureUnits) {
        LOGE("Too many combined texture units are being used");
        LOGE("GPU supports %d combined texture units", Hardware::maxCombinedTextureUnits);
    }

    return ++m_textureUnit;
}

void RenderState::releaseTextureUnit() {
    m_textureUnit--;
}

int RenderState::currentTextureUnit() {
    return m_textureUnit;
}

void RenderState::resetTextureUnit() {
    m_textureUnit = -1;
}

} // namespace Tangram
