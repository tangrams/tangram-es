#include "mbtilesTileTask.h"

#include "log.h"
#include "data/dataSource.h"
#include "hash-library/md5.cpp"

namespace Tangram {

MBTilesTileTask::MBTilesTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask) :
    DownloadTileTask(_tileId, _source, _subTask), m_db(_source->mbtilesDb()),
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

    char* data = rawTileData->data();
    size_t size = rawTileData->size();

    /**
     * We create an MD5 of the raw tile data. The MD5 functions as a hash
     * between the map and images tables. With this, tiles with duplicate
     * data will join to a single entry in the images table.
     */
    MD5 md5;
    std::string md5id = md5(data, size);

    try {
        m_putMapStmt.bind(1, z);
        m_putMapStmt.bind(2, m_tileId.x);
        m_putMapStmt.bind(3, y);
        m_putMapStmt.bind(4, md5id);
        m_putMapStmt.exec();
    } catch (std::exception& e) {
        LOGE("MBTiles SQLite put map statement failed: %s", e.what());
    }

    try {
        m_putImageStmt.bind(1, md5id);
        m_putImageStmt.bind(2, data, size);
        m_putImageStmt.exec();
    } catch (std::exception& e) {
        LOGE("MBTiles SQLite put image statement failed: %s", e.what());
    }

}

}
