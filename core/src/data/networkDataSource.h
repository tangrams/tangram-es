#pragma once

#include "data/dataSource.h"

namespace Tangram {

class NetworkDataSource : public RawDataSource {
public:
    NetworkDataSource(const std::string& _urlTemplate)
        : m_urlTemplate(_urlTemplate) {}

    bool loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;

    void cancelLoadingTile(const TileID& _tile) override;

private:
    /* Constructs the URL of a tile using <m_urlTemplate> */
    void constructURL(const TileID& _tileCoord, std::string& _url) const;

    std::string constructURL(const TileID& _tileCoord) const {
        std::string url;
        constructURL(_tileCoord, url);
        return url;
    }

    void removePending(const TileID& _tileId);

    // URL template for requesting tiles from a network or filesystem
    std::string m_urlTemplate;

    std::vector<TileID> m_pending;

    size_t m_maxDownloads = 4;

    std::mutex m_mutex;
};

}
