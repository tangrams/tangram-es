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
    std::shared_ptr<SQLite::Database> db = source().mbtilesDb();
    if (!db) return false;

    try {
        SQLite::Statement query(*db, "SELECT tile_data FROM tiles WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?;");

        // Google TMS to WMTS
        int z = m_tileId.z;
        int ymax = 1 << z;
        int y = ymax - m_tileId.y - 1;

        query.bind(1, z);
        query.bind(2, m_tileId.x);
        query.bind(3, y);
        if (query.executeStep()) {
            rawTileData = std::make_shared<std::vector<char>>();
            SQLite::Column column = query.getColumn(0);
            const char* blob = (const char*) column.getBlob();
            const int length = column.getBytes();
            rawTileData->resize(length);
            memcpy(rawTileData->data(), blob, length);
            source().cachePut(m_tileId, rawTileData);
            return true;
        }

    } catch (std::exception& e) {
        LOGE("MBTiles SQLite tile_data query failed: %s", e.what());
    }

    return false;
}

}
