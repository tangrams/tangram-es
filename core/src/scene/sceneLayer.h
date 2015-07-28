#pragma once

#include "data/filters.h"
#include "style/styleParamMap.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace Tangram {

using layerid = uint8_t; // allows maximum of 256 layers

class SceneLayer {
    std::vector<std::shared_ptr<SceneLayer>> m_subLayers;
    StyleParamMap m_styleParams;
    std::string m_name;
    layerid m_id;
    Filter m_filter;

    static layerid s_layerCount;

public:

    static constexpr size_t MAX_LAYERS = 1 << (sizeof(layerid) * 8);

    SceneLayer(const std::vector<std::shared_ptr<SceneLayer>> _subLayers, const StyleParamMap&& _styleParamMap, const std::string _name, Filter _filter);

    layerid getID() const { return m_id; }
    Filter getFilter() const { return m_filter; }
    std::string getName() const { return m_name; }
    StyleParamMap& getStyleParamMap() { return m_styleParams; }
    std::vector< std::shared_ptr<SceneLayer> >& getSublayers() { return m_subLayers; }
};

}

