#pragma once

#include <SQLiteCpp/Database.h>
#include "tileTask.h"

namespace Tangram {

/**
 * The MBTilesTileTask is where we get and put our tile data when there is an mbtiles
 * parameter specified in a data source.
 *
 * MBTiles Reference Implementation:
 *
 * https://github.com/mapbox/node-mbtiles/
 */
class MBTilesTileTask : public DownloadTileTask {
public:
    MBTilesTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask);

    virtual void process(TileBuilder& _tileBuilder) override;

private:
    bool getMBTilesData();
    void putMBTilesData();

    SQLite::Database& m_db;

    // SELECT statement from tiles view
    SQLite::Statement m_getTileDataStmt;

    // REPLACE INTO statement in map table
    SQLite::Statement m_putMapStmt;

    // REPLACE INTO statement in images table
    SQLite::Statement m_putImageStmt;

};

}
