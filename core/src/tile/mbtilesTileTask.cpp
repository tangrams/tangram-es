#include "mbtilesTileTask.h"

#include "platform.h"
#include "data/dataSource.h"

namespace Tangram {

MBTilesTileTask::MBTilesTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask) :
    m_db(_source->mbtilesDb()), DownloadTileTask(_tileId, _source, _subTask),
    m_getTileDataStmt(m_db, "SELECT tile_data FROM tiles WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?;"),
    m_putMapStmt(m_db, "REPLACE INTO map (zoom_level, tile_column, tile_row, tile_id) VALUES (?, ?, ?, ?);"),
    m_putImageStmt(m_db, "REPLACE INTO images (tile_id, tile_data) VALUES (?, ?);") {

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
        // https://github.com/mapbox/node-mbtiles/blob/4bbfaf991969ce01c31b95184c4f6d5485f717c3/lib/mbtiles.js#L149
        int z = m_tileId.z;
        int y = (1 << z) - 1 - m_tileId.y;

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
        LOGE("MBTiles SQLite get tile_data statement failed: %s", e.what());
    }

    return false;
}

void MBTilesTileTask::putMBTilesData() {
    int z = m_tileId.z;
    int y = (1 << z) - 1 - m_tileId.y;

    // TODO: Replace with MD5 Hash
    std::string str = std::to_string(z) + std::to_string(m_tileId.x) + std::to_string(y);
    const char* id = str.c_str();

    try {
        m_putMapStmt.bind(1, z);
        m_putMapStmt.bind(2, m_tileId.x);
        m_putMapStmt.bind(3, y);
        m_putMapStmt.bind(4, id);
        int rowsModified = m_putMapStmt.exec();
        LOGN("m_putMapStmt rows modified: %d", rowsModified);
    } catch (std::exception& e) {
        LOGE("MBTiles SQLite put map statement failed: %s", e.what());
    }

    try {
        m_putImageStmt.bind(1, id);
        m_putImageStmt.bind(2, rawTileData->data(), rawTileData->size());
        int rowsModified = m_putImageStmt.exec();
        LOGN("m_putImageStmt rows modified: %d", rowsModified);
    } catch (std::exception& e) {
        LOGE("MBTiles SQLite put image statement failed: %s", e.what());
    }

}

}
