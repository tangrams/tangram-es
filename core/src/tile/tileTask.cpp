#include "tileTask.h"
#include "data/dataSource.h"

namespace Tangram {

std::shared_ptr<TileData> TileTask::process(MapProjection& _projection) {
    return source->parse(tileId, _projection, *rawTileData);
}

}
