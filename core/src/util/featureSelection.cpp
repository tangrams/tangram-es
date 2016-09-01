#include "featureSelection.h"
#include "data/tileData.h"
#include "scene/sceneLayer.h"
#include "gl/renderState.h"
#include "gl/error.h"

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

    m_framebuffer->applyAsRenderTarget(_rs, 512, 512, 512, 512);

}

void FeatureSelection::endRenderPass(Tangram::RenderState& _rs) {

    // TODO: set inside framebuffer class unbind render target
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

}

}
