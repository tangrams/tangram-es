#include "data/networkDataSource.h"

#include "log.h"
#include "platform.h"

namespace Tangram {

NetworkDataSource::NetworkDataSource(Platform& _platform, std::string url, UrlOptions options) :
    m_platform(_platform),
    m_urlTemplate(std::move(url)),
    m_options(std::move(options)) {}

std::string NetworkDataSource::tileCoordinatesToQuadKey(const TileID &tile) {
    std::string quadKey;
    for (int i = tile.z; i > 0; i--) {
        char digit = '0';
        int mask = 1 << (i - 1);
        if ((tile.x & mask) != 0) {
            digit++;
        }
        if ((tile.y & mask) != 0) {
            digit++;
            digit++;
        }
        quadKey.push_back(digit);
    }
    return quadKey;
}

bool NetworkDataSource::urlHasTilePattern(const std::string &url) {
    return (url.find("{x}") != std::string::npos &&
            url.find("{y}") != std::string::npos &&
            url.find("{z}") != std::string::npos) ||
           (url.find("{q}") != std::string::npos);
}

std::string NetworkDataSource::buildUrlForTile(const TileID& tile, const std::string& urlTemplate, const UrlOptions& options, int subdomainIndex) {

    std::string url = urlTemplate;

    size_t xPos = url.find("{x}");
    if (xPos != std::string::npos) {
        url.replace(xPos, 3, std::to_string(tile.x));
    }
    size_t yPos = url.find("{y}");
    if (yPos != std::string::npos) {
        int y = tile.y;
        int z = tile.z;
        if (options.isTms) {
            // Convert XYZ to TMS
            y = (1 << z) - 1 - tile.y;
        }
        url.replace(yPos, 3, std::to_string(y));
    }
    size_t zPos = url.find("{z}");
    if (zPos != std::string::npos) {
        url.replace(zPos, 3, std::to_string(tile.z));
    }
    if (subdomainIndex < options.subdomains.size()) {
        size_t sPos = url.find("{s}");
        if (sPos != std::string::npos) {
            url.replace(sPos, 3, options.subdomains[subdomainIndex]);
        }
    }
    size_t qPos = url.find("{q}");
    if (qPos != std::string::npos) {
        auto quadkey = tileCoordinatesToQuadKey(tile);
        url.replace(qPos, 3, quadkey);
    }

    return url;
}

bool NetworkDataSource::loadTileData(std::shared_ptr<TileTask> task, TileTaskCb callback) {

    if (task->rawSource != this->level) {
        LOGE("NetworkDataSource must be last!");
        return false;
    }

    auto tileId = task->tileId();

    Url url(buildUrlForTile(tileId, m_urlTemplate, m_options, m_urlSubdomainIndex));

    if (!m_options.subdomains.empty()) {
        m_urlSubdomainIndex = (m_urlSubdomainIndex + 1) % m_options.subdomains.size();
    }

    LOGTInit(">>> %s", task->tileId().toString().c_str());
    UrlCallback onRequestFinish = [=](UrlResponse&& response) mutable {
        auto source = task->source();
        if (!source) {
            LOGW("URL Callback for deleted TileSource '%s'", url.string().c_str());
            return;
        }
        LOGT("<<< %s -- canceled:%d", task->tileId().toString().c_str(), task->isCanceled());

        if (task->isCanceled()) {
            return;
        }

        if (response.error) {
            LOGD("URL request '%s': %s", url.string().c_str(), response.error);

        } else if (!response.content.empty()) {
            auto& dlTask = static_cast<BinaryTileTask&>(*task);
            dlTask.rawTileData = std::make_shared<std::vector<char>>(std::move(response.content));
        }
        callback.func(std::move(task));
    };

    auto& dlTask = static_cast<BinaryTileTask&>(*task);
    dlTask.urlRequestHandle = m_platform.startUrlRequest(url, std::move(onRequestFinish));
    dlTask.urlRequestStarted = true;

    return true;
}

void NetworkDataSource::cancelLoadingTile(TileTask& task) {
    auto& dlTask = static_cast<BinaryTileTask&>(task);
    if (dlTask.urlRequestStarted) {
        dlTask.urlRequestStarted = false;

        m_platform.cancelUrlRequest(dlTask.urlRequestHandle);
    }
}

}
