#include "tile/tileTask.h"

#include "data/tileSource.h"
#include "scene/scene.h"
#include "tile/tile.h"
#include "tile/tileBuilder.h"
#include "util/mapProjection.h"

namespace Tangram {

TileTask::TileTask(TileID& _tileId, Scene& _scene, TileSource& _source, int _subTask) :
    m_tileId(_tileId),
    m_subTaskId(_subTask),
    m_scene(_scene.weak_ptr()),
    m_sourceId(_source.id()),
    m_sourceGeneration(_source.generation()),
    m_ready(false),
    m_canceled(false),
    m_needsLoading(true),
    m_priority(0),
    m_proxyState(false) {}

TileTask::~TileTask() {}

TileSource& TileTask::source() {
    auto scene = m_scene.lock();
    assert(scene);
    return *scene->getTileSource(m_sourceId);
}

std::unique_ptr<Tile> TileTask::getTile() {
    return std::move(m_tile);
}

void TileTask::setTile(std::unique_ptr<Tile>&& _tile) {
    m_tile = std::move(_tile);
    m_ready = true;
}

void TileTask::process(TileBuilder& _tileBuilder) {
    auto scene = m_scene.lock();
    if (!scene) { return; }

    auto source = scene->getTileSource(m_sourceId);
    if (!source) { return; }

    auto tileData = source->parse(*this);
    if (!tileData) {
        cancel();
        return;
    }

    m_tile = _tileBuilder.build(m_tileId, *tileData, *source);
    m_ready = true;

    scene->requestRender();
}

void TileTask::complete() {

    for (auto& subTask : m_subTasks) {
        assert(subTask->isReady());
        subTask->complete(*this);
    }

}

}
