#pragma once

#include "data/properties.h"
#include "gl/framebuffer.h"
#include "tile/tileID.h"
#include "util/fastmap.h"
#include <mutex>
#include <map>

namespace Tangram {

struct Feature;
class SceneLayer;
class RenderState;

class FeatureSelection {

public:

    FeatureSelection();

    uint32_t colorIdentifier(const Feature& _feature, const TileID& _tileID);

    // TODO: return true when pending requests for a
    // screen location has been done from the tangram API
    bool pendingRequests() const { return true; }

    bool beginRenderPass(RenderState& _rs);

    void endRenderPass(RenderState& _rs);

    bool clearFeaturesForTile(const TileID& _tileID);

    GLuint readBufferAt(RenderState& _rs, float _x, float _y, int _vpWidth, int _vpHeight) const;

private:

    using Entries = std::map<uint32_t, std::shared_ptr<Properties>>;

    uint32_t m_entry;

    std::unique_ptr<FrameBuffer> m_framebuffer;

    fastmap<TileID, Entries> m_tileFeatures;

    std::mutex m_mutex;

};

}
