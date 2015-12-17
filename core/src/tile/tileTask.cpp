#include "tileTask.h"
#include "data/dataSource.h"

namespace Tangram {

TileTask::TileTask(TileID& _tileId, std::shared_ptr<DataSource> _source) :
    m_tileId(_tileId),
    m_source(_source),
    m_sourceGeneration(_source->generation()) {}

}
