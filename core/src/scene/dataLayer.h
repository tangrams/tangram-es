#pragma once

#include "scene/sceneLayer.h"

#include <string>

namespace Tangram {

// DataLayer represents a top-level layer in the stylesheet, distinct from
// SceneLayer by its association with a collection within a TileSource
class DataLayer : public SceneLayer {

    std::string m_source;
    std::vector<std::string> m_collections;

public:

    DataLayer(SceneLayer _layer, const std::string& _source, const std::vector<std::string>& _collections);

    const auto& source() const { return m_source; }
    const auto& collections() const { return m_collections; }

};

}
