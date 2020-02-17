#pragma once

#include "data/tileSource.h"
#include "platform.h"

#include <unordered_map>

namespace Tangram {

class Platform;

class NetworkDataSource : public TileSource::DataSource {
public:

    struct UrlOptions {
        std::vector<std::string> subdomains;
        bool isTms = false;
    };

    NetworkDataSource(Platform& _platform, std::string url, UrlOptions options);

    bool loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;

    void cancelLoadingTile(TileTask& _task) override;

    static std::string tileCoordinatesToQuadKey(const TileID& tile);

    static std::string buildUrlForTile(const TileID& tile, const std::string& urlTemplate, const UrlOptions& options, int subdomainIndex);

private:

    Platform& m_platform;

    std::string m_urlTemplate;

    UrlOptions m_options;

    int m_urlSubdomainIndex = 0;
};

}
