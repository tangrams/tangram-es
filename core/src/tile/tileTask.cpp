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
    m_priority(0) {}

void TileTask::process(TileBuilder& _tileBuilder) {

    auto tileData = m_source->parse(*this, *_tileBuilder.scene().mapProjection());

    if (tileData) {
        m_tile = _tileBuilder.build(m_tileId, *tileData, *m_source);
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
