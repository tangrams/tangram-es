#include "mbtilesTileTask.h"

namespace Tangram {

/**
 * An MBTilesTileTask is created when we have a path to an MBTiles file for the DataSource.
 * Rather than assuming we have already gotten our rawData to parse, we check to see if
 * we have rawData. If not, we query the SQLite MBTiles database for the rawData that we seek.
 *
 * @param _tileBuilder
 */
void MBTilesTileTask::process(TileBuilder &_tileBuilder) {
    if (hasData()) {
        TileTask::process(_tileBuilder);
        return;
    }

    if (loadMBTilesData()) {
        TileTask::process(_tileBuilder);
    } else {
        cancel();
    }
}

bool MBTilesTileTask::loadMBTilesData() {
    return false;
}

}
