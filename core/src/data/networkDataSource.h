#pragma once

#include "data/tileSource.h"
#include "platform.h"

#include <unordered_map>

namespace Tangram {

class Platform;

class NetworkDataSource : public TileSource::DataSource {
public:

    NetworkDataSource(std::shared_ptr<Platform> _platform, const std::string& _urlTemplate,
                      std::vector<std::string>&& _urlSubdomains, bool _isTms);

    TileID getFallbackTileID(const TileID& _tileID, int32_t _maxZoom, int32_t _zoomBias) override { return _tileID; }

    bool loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;

    void cancelLoadingTile(TileTask& _task) override;

private:
    // Build the URL of a tile using our URL template.
    std::string buildUrlForTile(const TileID& _tile, size_t _subdomainIndex) const;

    std::shared_ptr<Platform> m_platform;

    // URL template for requesting tiles from a network or filesystem
    std::string m_urlTemplate;
    std::vector<std::string> m_urlSubdomains;
    size_t m_urlSubdomainIndex = 0;
    bool m_isTms = false;
};

}
