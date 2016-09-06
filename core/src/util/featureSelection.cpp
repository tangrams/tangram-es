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

uint32_t FeatureSelection::colorIdentifier(const Feature& _feature, const TileID& _tileID) {

    std::lock_guard<std::mutex> guard(m_mutex);

    m_entry++;

    auto& tileFeatures = m_tileFeatures[_tileID];
    tileFeatures[m_entry] = std::make_shared<Properties>(_feature.props);

    return m_entry;
}

bool FeatureSelection::beginRenderPass(Tangram::RenderState& _rs) {

    _rs.saveFramebufferState();

    return m_framebuffer->applyAsRenderTarget(_rs, {0.0, 0.0, 0.0, 0.0}, 256, 256);
}

void FeatureSelection::endRenderPass(Tangram::RenderState& _rs) {

    _rs.applySavedFramebufferState();

}

bool FeatureSelection::clearFeaturesForTile(const Tangram::TileID& _tileID) {

    auto it = m_tileFeatures.find(_tileID);
    if (it != m_tileFeatures.end()) {
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_tileFeatures.erase(it);
        }

        return true;
    }

    return false;
}

GLuint FeatureSelection::readBufferAt(RenderState& _rs, float _x, float _y,
                                      int _vpWidth, int _vpHeight) const {

    glm::vec2 fbPosition((_x / _vpWidth) * m_framebuffer->getWidth(),
                        (1.f - (_y / _vpHeight)) * m_framebuffer->getHeight());

    _rs.saveFramebufferState();

    m_framebuffer->bind(_rs);

    GLuint pixel;
    GL_CHECK(glReadPixels(floorf(fbPosition.x), floorf(fbPosition.y),
                          1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel));

    _rs.applySavedFramebufferState();

    for (const auto& tileFeatures : m_tileFeatures) {
        auto it = tileFeatures.second.find(pixel);
        if (it != tileFeatures.second.end()) {
            std::shared_ptr<Properties> props = it->second;
            if (props->contains("name")) {
                LOGS("props name: %s", props->getString("name").c_str());
            }
        }
    }

    return pixel;
}

}
