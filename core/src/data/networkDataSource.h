#pragma once

#include "data/tileSource.h"
#include "platform.h"

#include <unordered_map>

namespace Tangram {

class Platform;

class NetworkDataSource : public TileSource::DataSource {
public:

    NetworkDataSource(std::shared_ptr<Platform> _platform, const std::string& _urlTemplate,
            std::vector<std::string>&& _urlSubdomains, bool isTms);

    bool loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;

    void cancelLoadingTile(const TileID& _tile) override;

private:
    // Build the URL of a tile using our URL template.
    std::string buildUrlForTile(const TileID& tile, size_t subdomainIndex) const;

    // Each pending tile request is stored as a pair of TileID and UrlRequestHandle.
    struct TileRequest {
        TileID tile;
        UrlRequestHandle request;
    };

    std::vector<TileRequest> m_pending;

    // Remove a pending list item with the given TileID if present, and if cancelRequest is true
    // also cancels the corresponding URL request.
    void removePending(const TileID& tile, bool cancelRequest);

    std::shared_ptr<Platform> m_platform;

    // URL template for requesting tiles from a network or filesystem
    std::string m_urlTemplate;
    std::vector<std::string> m_urlSubdomains;
    size_t m_urlSubdomainIndex = 0;
    bool m_isTms = false;

    std::mutex m_mutex;

};

}
