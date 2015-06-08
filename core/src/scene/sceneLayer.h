#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "filters.h"
#include "style/styleParamMap.h"

namespace Tangram {

    class SceneLayer {
        std::vector<SceneLayer*> m_subLayers;
        /*
         * void* could be any value corresponding to the the named style property key
         */
        StyleParamMap m_styleParams;
        std::string m_name;
        long long m_id;
        Filter* m_filter;

        static short s_layerCount;

    public:
        SceneLayer(const std::vector<SceneLayer*>&& _subLayers, const StyleParamMap&& _styleParamMap, const std::string _name, Filter* _filter);
        ~SceneLayer();

        long long getID() const { return m_id; }
        Filter* getFilter() const { return m_filter; }
        std::string getName() const { return m_name; }
        StyleParamMap& getStyleParamMap() { return m_styleParams; }
    };

}

