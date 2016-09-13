#pragma once

#include <SQLiteCpp/Database.h>
#include "tileTask.h"

namespace Tangram {

class MBTilesTileTask : public DownloadTileTask {
public:
    MBTilesTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask);

    virtual void process(TileBuilder& _tileBuilder) override;

private:
    bool getMBTilesData();
    void putMBTilesData();

    SQLite::Database& m_db;
    SQLite::Statement m_getTileDataStmt;
//    SQLite::Statement m_putTileDataStmt;
};

}
