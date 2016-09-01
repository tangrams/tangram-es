#include "featureSelection.h"
#include "data/tileData.h"
#include "scene/sceneLayer.h"
#include "gl/renderState.h"
#include "gl/error.h"
#include "glm/vec2.hpp"
#include "debug/textDisplay.h"

namespace Tangram {

FeatureSelection::FeatureSelection() :
    m_entry(0) {

    m_framebuffer = std::make_unique<FrameBuffer>();
}

uint32_t FeatureSelection::colorIdentifier(const Feature& _feature, const SceneLayer& _layer) const {
    m_entry++;

    uint32_t color = m_entry;
    color = color | 0xff000000;

    return color;
}

void FeatureSelection::beginRenderPass(Tangram::RenderState& _rs) {

    _rs.saveFramebufferState();

    m_framebuffer->applyAsRenderTarget(_rs, {0.0, 0.0, 0.0, 0.0}, 256, 256);

}

void FeatureSelection::endRenderPass(Tangram::RenderState& _rs) {

    _rs.applySavedFramebufferState();

}

GLuint FeatureSelection::readBufferAt(RenderState& _rs, float _x, float _y, int _vpWidth, int _vpHeight) const {

    glm::vec2 fbPosition((_x / _vpWidth) * m_framebuffer->getWidth(),
                        (1.f - (_y / _vpHeight)) * m_framebuffer->getHeight());

    _rs.saveFramebufferState();

    m_framebuffer->bind(_rs);

    GLuint pixel;
    GL_CHECK(glReadPixels(fbPosition.x, fbPosition.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel));

    _rs.applySavedFramebufferState();

    return pixel;
}

}
