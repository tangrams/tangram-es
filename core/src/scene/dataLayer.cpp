#include "dataLayer.h"

namespace Tangram {

DataLayer::DataLayer(SceneLayer _layer, const std::string& _source, const std::vector<std::string>& _collections) :
    SceneLayer(_layer),
    m_source(_source),
    m_collections(_collections) {}

}
