#include "sceneLayer.h"

namespace Tangram {

    short SceneLayer::s_layerCount = 0;

    SceneLayer::SceneLayer(const std::vector<SceneLayer*>&& _subLayers, const StyleParamMap&& _styleParamMap, const std::string _name, Filter* _filter) :
        m_subLayers(std::move(_subLayers)), m_styleParams(std::move(_styleParamMap)), m_name(_name), m_filter(_filter) {

        m_id = s_layerCount++;

    }

    SceneLayer::~SceneLayer() {
        for(auto& subLayer : m_subLayers) {
            delete subLayer;
        }
        delete m_filter;
    }
}

