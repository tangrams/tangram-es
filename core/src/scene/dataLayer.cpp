#include "scene/dataLayer.h"

namespace Tangram {

DataLayer::DataLayer(SceneLayer layer, std::string source, std::vector<std::string> collections) :
    SceneLayer(std::move(layer)),
    m_source(std::move(source)),
    m_collections(std::move(collections)) {}

}
