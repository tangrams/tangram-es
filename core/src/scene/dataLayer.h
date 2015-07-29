#pragma once

#include "scene/sceneLayer.h"
#include <memory>
#include <string>

namespace Tangram {

class DataSource;

// DataLayer represents a top-level layer in the stylesheet, distinct from
// SceneLayer by its association with a collection within a DataSource
class DataLayer {

    SceneLayer m_layer;

    std::shared_ptr<DataSource> m_source;
    std::string m_collection;

public:

    DataLayer(std::shared_ptr<DataSource> _source, const std::string& _collection, const SceneLayer& _layer);

    const auto& layer() const { return m_layer; }
    const auto& source() const { return m_source; }
    const auto& collection() const { return m_collection; }

};

}
