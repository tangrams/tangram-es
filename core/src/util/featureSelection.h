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

    bool clearFeaturesForTile(const TileID& _tileID);

    std::shared_ptr<Properties> featurePropertiesForEntry(uint32_t entry) const;

private:

    using Entries = std::map<uint32_t, std::shared_ptr<Properties>>;

    uint32_t m_entry;

    fastmap<TileID, Entries> m_tileFeatures;

    std::mutex m_mutex;

};

}
