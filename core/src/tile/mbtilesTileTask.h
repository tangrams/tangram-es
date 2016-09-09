#pragma once

#include "tileTask.h"

namespace Tangram {

class MBTilesTileTask : public DownloadTileTask {
public:
    MBTilesTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask)
            : DownloadTileTask(_tileId, _source, _subTask) {}

    virtual void process(TileBuilder& _tileBuilder) override;

private:
    bool loadMBTilesData();

};

}
