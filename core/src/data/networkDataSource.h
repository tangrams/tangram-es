#pragma once

#include "data/tileSource.h"

namespace Tangram {

class Platform;

class NetworkDataSource : public TileSource::DataSource {
public:

    NetworkDataSource(std::shared_ptr<Platform> _platform, const std::string& _urlTemplate,
            std::vector<std::string>&& _urlSubdomains);

    bool loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;

    void cancelLoadingTile(const TileID& _tile) override;

private:
    /* Constructs the URL of a tile using <m_urlTemplate> */
    std::string constructURL(const TileID& _tileCoord, size_t index) const;

    void removePending(const TileID& _tileId);

    std::shared_ptr<Platform> m_platform;

    // URL template for requesting tiles from a network or filesystem
    std::string m_urlTemplate;
    std::vector<std::string> m_urlSubdomains;
    size_t m_urlSubdomainIndex = 0;

    std::vector<TileID> m_pending;

    size_t m_maxDownloads;

    std::mutex m_mutex;

};

}
