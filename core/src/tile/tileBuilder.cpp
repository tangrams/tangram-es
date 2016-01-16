#include "tile/tileBuilder.h"

#include "gl/vboMesh.h"

#include "data/dataSource.h"

#include "scene/dataLayer.h"
#include "scene/scene.h"
#include "style/style.h"
#include "tile/tile.h"


namespace Tangram {

TileBuilder::TileBuilder() {}

TileBuilder::~TileBuilder() {}

StyleBuilder* TileBuilder::getStyleBuilder(const std::string& _name) {
    auto it = m_styleBuilder.find(_name);
    if (it == m_styleBuilder.end()) { return nullptr; }

    return it->second.get();
}

void TileBuilder::setScene(std::shared_ptr<Scene> _scene) {

    m_scene = _scene;

    m_styleContext.setScene(*_scene);

    // Initialize StyleBuilders
    m_styleBuilder.clear();
    for (auto& style : _scene->styles()) {
        m_styleBuilder[style->getName()] = style->createBuilder();
    }
}

void TileBuilder::begin(const TileID& _tileID, const DataSource& _source) {
    m_tile = std::make_shared<Tile>(_tileID, *m_scene->mapProjection(), &_source);

    m_styleContext.setGlobalZoom(_tileID.z);

    for (auto& builder : m_styleBuilder) {
        if (builder.second)
            builder.second->begin(*m_tile);
    }
}

bool TileBuilder::beginLayer(const std::string& _layerName) {

    m_activeLayers.clear();

    for (auto& layer : m_scene->layers()) {

        if (!_layerName.empty()) {
            const auto& dlc = layer.collections();
            bool layerContainsCollection =
                std::find(dlc.begin(), dlc.end(), _layerName) != dlc.end();

            if (!layerContainsCollection) {
                continue;
            }
            m_activeLayers.push_back(&layer);
        }
    }
    // LOGE("begin layer %d - %s", m_activeLayers.size(), _layerName.c_str());

    return !m_activeLayers.empty();
}

bool TileBuilder::matchFeature(const Feature& _feature) {
    // for (auto* layer : m_activeLayers) {
    //     m_ruleSet.apply(_feature, *layer, m_styleContext, *this);
    // }
    return true;
}


void TileBuilder::addFeature(const Feature& _feature) {
    for (auto* layer : m_activeLayers) {
        m_ruleSet.apply(_feature, *layer, m_styleContext, *this);
    }
}

std::shared_ptr<Tile> TileBuilder::build() {
    for (auto& builder : m_styleBuilder) {
        m_tile->getMesh(builder.second->style()) = builder.second->build();
    }
    return m_tile;
}

}
