#include "featureSelection.h"
#include "data/tileData.h"
#include "scene/sceneLayer.h"
#include "gl/renderState.h"
#include "gl/error.h"
#include "glm/vec2.hpp"
#include "debug/textDisplay.h"
#include "data/properties.h"
#include "data/propertyItem.h"

namespace Tangram {

FeatureSelection::FeatureSelection() :
    m_entry(0) {

    m_framebuffer = std::make_unique<FrameBuffer>();
}

uint32_t FeatureSelection::colorIdentifier(const Feature& _feature,
                                           const SceneLayer& _layer) {

    std::lock_guard<std::mutex> guard(m_mutex);

    m_entry++;

    uint32_t color = m_entry;
    color = color | 0xff000000;

    m_props[m_entry] = std::make_shared<Properties>(_feature.props);

    return color;
}

bool FeatureSelection::beginRenderPass(Tangram::RenderState& _rs) {

    _rs.saveFramebufferState();

    return m_framebuffer->applyAsRenderTarget(_rs, {0.0, 0.0, 0.0, 0.0}, 256, 256);
}

void FeatureSelection::endRenderPass(Tangram::RenderState& _rs) {

    _rs.applySavedFramebufferState();

}

GLuint FeatureSelection::readBufferAt(RenderState& _rs, float _x, float _y,
                                      int _vpWidth, int _vpHeight) const {

    glm::vec2 fbPosition((_x / _vpWidth) * m_framebuffer->getWidth(),
                        (1.f - (_y / _vpHeight)) * m_framebuffer->getHeight());

    _rs.saveFramebufferState();

    m_framebuffer->bind(_rs);

    GLuint pixel;
    GL_CHECK(glReadPixels(fbPosition.x, fbPosition.y, 1, 1, GL_RGBA,
                          GL_UNSIGNED_BYTE, &pixel));

    _rs.applySavedFramebufferState();

    uint32_t entry = 0x00ffffff & pixel;

    auto it = m_props.find(entry);
    if (it != m_props.end()) {
        std::shared_ptr<Properties> props = it->second;
        if (props->contains("name")) {
            LOGS("props name: %s", props->getString("name").c_str());
        }
    }

    return pixel;
}

}
