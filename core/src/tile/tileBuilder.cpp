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

void TileBuilder::setScene(std::shared_ptr<Scene> _scene) {

    m_scene = _scene;

    m_styleContext.initFunctions(*_scene);
}

std::shared_ptr<Tile> TileBuilder::build(TileID _tileID, const TileData& _tileData,
                                         const DataSource& _source) {

    m_tile = std::make_shared<Tile>(_tileID, *m_scene->mapProjection(), &_source);
    m_tile->initGeometry(m_scene->styles().size());

    m_styleContext.setGlobalZoom(_tileID.s);

    for (auto& style : m_scene->styles()) {
        style->onBeginBuildTile(*m_tile);
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

    for (auto& style : m_scene->styles()) {
        style->onEndBuildTile(*m_tile);
        auto& mesh = m_tile->getMesh(*style);
        if (mesh) {
            mesh->compileVertexBuffer();
        }
    }

    return std::move(m_tile);
}

}
