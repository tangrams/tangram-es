#include "tileTask.h"
#include "data/dataSource.h"
#include "tile/tileBuilder.h"
#include "scene/scene.h"
#include "util/mapProjection.h"
#include "tile/tile.h"

namespace Tangram {

TileTask::TileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask) :
    m_tileId(_tileId),
    m_subTaskId(_subTask),
    m_source(_source),
    m_sourceGeneration(_source->generation()),
    m_priority(0) {}

void TileTask::process(TileBuilder& _tileBuilder) {
    m_tile = _tileBuilder.build(*this);
}

void TileTask::complete() {

    for (auto& subTask : m_subTasks) {
        assert(subTask->isReady());
        subTask->complete(*this);
    }
    
}

}
