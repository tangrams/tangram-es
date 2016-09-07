#include "featureSelection.h"
#include "data/tileData.h"
#include "scene/sceneLayer.h"
#include "gl/renderState.h"
#include "gl/error.h"
#include "glm/vec2.hpp"
#include "debug/textDisplay.h"
#include "data/properties.h"
#include "data/propertyItem.h"
#include "log.h"

namespace Tangram {

FeatureSelection::FeatureSelection() :
    m_entry(0) {
}

uint32_t FeatureSelection::colorIdentifier(const Feature& _feature, const TileID& _tileID) {

    std::lock_guard<std::mutex> guard(m_mutex);

    m_entry++;

    auto& tileFeatures = m_tileFeatures[_tileID];
    tileFeatures[m_entry] = std::make_shared<Properties>(_feature.props);

    return m_entry;
}

bool FeatureSelection::clearFeaturesForTile(const TileID& _tileID) {

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

void FeatureSelection::featureForEntry(uint32_t entry) const {

    for (const auto& tileFeatures : m_tileFeatures) {
        auto it = tileFeatures.second.find(entry);
        if (it != tileFeatures.second.end()) {
            std::shared_ptr<Properties> props = it->second;
            if (props->contains("name")) {
                LOGS("props name: %s", props->getString("name").c_str());
            }
        }
    }
}

}
