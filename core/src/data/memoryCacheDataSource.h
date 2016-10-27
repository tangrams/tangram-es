#pragma once

#include "data/tileSource.h"

namespace Tangram {

class MemoryCacheDataSource : public TileSource::DataSource {
public:

    MemoryCacheDataSource();
    ~MemoryCacheDataSource();

    bool loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;

    void clear() override;

    /* @_cacheSize: Set size of in-memory cache for tile data in bytes.
     * This cache holds unprocessed tile data for fast recreation of recently used tiles.
     */
    void setCacheSize(size_t _cacheSize);

private:
    bool cacheGet(BinaryTileTask& _task);

    void cachePut(const TileID& _tileID, std::shared_ptr<std::vector<char>> _rawDataRef);

    std::unique_ptr<RawCache> m_cache;

};

}
