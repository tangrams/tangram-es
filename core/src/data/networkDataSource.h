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
    void constructURL(const TileID& _tileCoord, std::string& _url, int32_t index) const;

    std::string constructURL(const TileID& _tileCoord, int32_t index) const {
        std::string url;
        constructURL(_tileCoord, url, index);
        return url;
    }

    void removePending(const TileID& _tileId);

    std::shared_ptr<Platform> m_platform;

    // URL template for requesting tiles from a network or filesystem
    std::string m_urlTemplate;
    std::vector<std::string> m_urlSubdomains;
    int32_t m_urlIndex = -1;

    std::vector<TileID> m_pending;

    size_t m_maxDownloads;

    std::mutex m_mutex;

};

}
