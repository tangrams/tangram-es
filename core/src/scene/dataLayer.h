#pragma once

#include "scene/sceneLayer.h"
#include <string>

namespace Tangram {

// DataLayer represents a top-level layer in the stylesheet, distinct from
// SceneLayer by its association with a collection within a DataSource
class DataLayer : public SceneLayer {

    std::string m_source;
    std::string m_collection;

public:

    DataLayer(SceneLayer _layer, const std::string& _source, const std::string& _collection);

    const auto& source() const { return m_source; }
    const auto& collection() const { return m_collection; }

};

}
