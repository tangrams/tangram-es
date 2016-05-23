#include "tile/tileBuilder.h"

#include "data/dataSource.h"
#include "gl/mesh.h"
#include "scene/dataLayer.h"
#include "scene/scene.h"
#include "style/style.h"
#include "tile/tile.h"

namespace Tangram {

TileBuilder::TileBuilder(std::shared_ptr<Scene> _scene)
    : m_scene(_scene) {

    m_styleContext.initFunctions(*_scene);

    // Initialize StyleBuilders
    for (auto& style : _scene->styles()) {
        m_styleBuilder[style->getName()] = style->createBuilder();
    }
}

TileBuilder::~TileBuilder() {}

StyleBuilder* TileBuilder::getStyleBuilder(const std::string& _name) {
    auto it = m_styleBuilder.find(_name);
    if (it == m_styleBuilder.end()) { return nullptr; }

    return it->second.get();
}

std::shared_ptr<Tile> TileBuilder::build(TileID _tileID, const TileData& _tileData, const DataSource& _source) {

    auto tile = std::make_shared<Tile>(_tileID, *m_scene->mapProjection(), &_source);

    tile->initGeometry(m_scene->styles().size());

    m_styleContext.setKeywordZoom(_tileID.s);

    for (auto& builder : m_styleBuilder) {
        if (builder.second)
            builder.second->setup(*tile);
    }

    for (const auto& datalayer : m_scene->layers()) {

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

    float tileScale = pow(2, _tileID.s - _tileID.z);

    m_labelLayout.setup(tileScale);
    LOG("tileScale %f", tileScale);

    for (auto& builder : m_styleBuilder) {

        builder.second->addLayoutItems(m_labelLayout);
    }

    m_labelLayout.process();

    for (auto& builder : m_styleBuilder) {
        tile->setMesh(builder.second->style(), builder.second->build());
    }

    return tile;
}

}
