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

std::shared_ptr<Tile> TileBuilder::build(TileID _tileID, const TileData& _tileData,
                                         const DataSource& _source) {

    auto tile = std::make_shared<Tile>(_tileID, *m_scene->mapProjection(), &_source);

    const auto& layers = m_scene->layers();

    m_styleContext.setGlobalZoom(_tileID.s);

    for (auto& builder : m_styleBuilder) {
        if (builder.second)
            builder.second->begin(*tile);
    }

    for (const auto& datalayer : layers) {

        if (datalayer.source() != _source.name()) { continue; }

        for (const auto& collection : _tileData.layers) {

            if (!collection.name.empty()) {
                const auto& dlc = datalayer.collections();
                bool layerContainsCollection =
                    std::find(dlc.begin(), dlc.end(), collection.name) != dlc.end();

                if (!layerContainsCollection) { continue; }
            }

            for (const auto& feat : collection.features) {
                m_ruleSet.apply(feat, datalayer, m_styleContext, *this);
            }
        }
    }

    for (auto& builder : m_styleBuilder) {
        tile->getMesh(builder.second->style()) = builder.second->build();
    }

    return tile;
}

}
