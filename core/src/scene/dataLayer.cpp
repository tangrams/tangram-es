#include "dataLayer.h"

namespace Tangram {

DataLayer::DataLayer(std::shared_ptr<DataSource> _source, const std::string& _collection, const SceneLayer& _layer) :
    m_layer(_layer),
    m_source(_source),
    m_collection(_collection) {}

}
