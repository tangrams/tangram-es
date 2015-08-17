#include "tileTask.h"
#include "data/dataSource.h"

namespace Tangram {

std::shared_ptr<TileData> TileTask::process() {
    return source->parse(*tile, *rawTileData);
}

}
