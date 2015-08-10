#include "dataLayer.h"

namespace Tangram {

DataLayer::DataLayer(SceneLayer _layer, const std::string& _source, const std::string& _collection) :
    SceneLayer(_layer),
    m_source(_source),
    m_collection(_collection) {}

}
