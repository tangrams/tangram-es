#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "filters.h"
#include "style/styleParamMap.h"

namespace Tangram {

    class SceneLayer {
        std::vector<std::shared_ptr<SceneLayer>> m_subLayers;
        StyleParamMap m_styleParams;
        std::string m_name;
        uint32_t m_id;
        Filter* m_filter;

        static uint32_t s_layerCount;

    public:
        SceneLayer(const std::vector<std::shared_ptr<SceneLayer>>&& _subLayers, const StyleParamMap&& _styleParamMap, const std::string _name, Filter* _filter);
        ~SceneLayer();

        uint32_t getID() const { return m_id; }
        Filter* getFilter() const { return m_filter; }
        std::string getName() const { return m_name; }
        StyleParamMap& getStyleParamMap() { return m_styleParams; }
        std::vector<std::shared_ptr<SceneLayer>>& getSublayers() { return m_subLayers; }
    };

}

