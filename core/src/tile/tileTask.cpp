#include "tile/tileTask.h"

#include "data/tileSource.h"
#include "scene/scene.h"
#include "tile/tile.h"
#include "tile/tileBuilder.h"
#include "util/mapProjection.h"

namespace Tangram {

TileTask::TileTask(TileID& _tileId, std::shared_ptr<TileSource> _source, int _subTask) :
    m_tileId(_tileId),
    m_subTaskId(_subTask),
    m_source(_source),
    m_sourceGeneration(_source->generation()),
    m_canceled(false),
    m_needsLoading(true),
    m_priority(0),
    m_proxyState(false) {}

void TileTask::process(TileBuilder& _tileBuilder) {

    auto tileData = m_source->parse(*this, *_tileBuilder.scene().mapProjection());

    if (tileData) {
        auto tile = _tileBuilder.build(m_tileId, *tileData, *m_source);
        std::atomic_store(&m_tile, tile);
    } else {
        cancel();
    }
}

void TileTask::complete() {

    for (auto& subTask : m_subTasks) {
        assert(subTask->isReady());
        subTask->complete(*this);
    }

}

}
