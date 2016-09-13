#include "mbtilesTileTask.h"

#include "platform.h"
#include "data/dataSource.h"

namespace Tangram {

MBTilesTileTask::MBTilesTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask) :
    m_db(_source->mbtilesDb()), DownloadTileTask(_tileId, _source, _subTask),
    m_getTileDataStmt(m_db, "SELECT tile_data FROM tiles WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?;") {

}

/**
 * An MBTilesTileTask is created when we have a path to an MBTiles file for the DataSource.
 * Rather than assuming we have already gotten our rawData to parse, we check to see if
 * we have rawData. If not, we query the SQLite MBTiles database for the rawData that we seek.
 *
 * @param _tileBuilder
 */
void MBTilesTileTask::process(TileBuilder &_tileBuilder) {
    if (hasData()) {
        // If the data did not come from the in-memory cache, it is new.
        // It should be added to the MBTiles.
        if (!dataFromCache) {
            putMBTilesData();
        }
        TileTask::process(_tileBuilder);
        return;
    }

    if (getMBTilesData()) {
        TileTask::process(_tileBuilder);
    } else {
        cancel();
    }
}

bool MBTilesTileTask::getMBTilesData() {
    try {
        // Google TMS to WMTS
        int z = m_tileId.z;
        int ymax = 1 << z;
        int y = ymax - m_tileId.y - 1;

        m_getTileDataStmt.bind(1, z);
        m_getTileDataStmt.bind(2, m_tileId.x);
        m_getTileDataStmt.bind(3, y);
        if (m_getTileDataStmt.executeStep()) {
            rawTileData = std::make_shared<std::vector<char>>();
            SQLite::Column column = m_getTileDataStmt.getColumn(0);
            const char* blob = (const char*) column.getBlob();
            const int length = column.getBytes();
            rawTileData->resize(length);
            memcpy(rawTileData->data(), blob, length);
            m_source->cachePut(m_tileId, rawTileData);
            return true;
        }

    } catch (std::exception& e) {
        LOGE("MBTiles SQLite tile_data query failed: %s", e.what());
    }

    return false;
}

void MBTilesTileTask::putMBTilesData() {
    
}

}
