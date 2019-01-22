#include "data/networkDataSource.h"

#include "log.h"
#include "platform.h"

namespace Tangram {

NetworkDataSource::NetworkDataSource(Platform& _platform, const std::string& _urlTemplate,
                                     std::vector<std::string>&& _urlSubdomains, bool isTms) :
    m_platform(_platform),
    m_urlTemplate(_urlTemplate),
    m_urlSubdomains(std::move(_urlSubdomains)),
    m_isTms(isTms) {}

std::string NetworkDataSource::buildUrlForTile(const TileID& tile, size_t subdomainIndex) const {

    std::string url = m_urlTemplate;

    size_t xPos = url.find("{x}");
    if (xPos != std::string::npos) {
        url.replace(xPos, 3, std::to_string(tile.x));
    }
    size_t yPos = url.find("{y}");
    if (yPos != std::string::npos) {
        int y = tile.y;
        int z = tile.z;
        if (m_isTms) {
            // Convert XYZ to TMS
            y = (1 << z) - 1 - tile.y;
        }
        url.replace(yPos, 3, std::to_string(y));
    }
    size_t zPos = url.find("{z}");
    if (zPos != std::string::npos) {
        url.replace(zPos, 3, std::to_string(tile.z));
    }
    if (subdomainIndex < m_urlSubdomains.size()) {
        size_t sPos = url.find("{s}");
        if (sPos != std::string::npos) {
            url.replace(sPos, 3, m_urlSubdomains[subdomainIndex]);
        }
    }
    return url;
}

bool NetworkDataSource::loadTileData(std::shared_ptr<TileTask> task, TileTaskCb callback) {

    if (task->rawSource != this->level) {
        LOGE("NetworkDataSource must be last!");
        return false;
    }

    auto tileId = task->tileId();

    Url url(buildUrlForTile(tileId, m_urlSubdomainIndex));

    if (!m_urlSubdomains.empty()) {
        m_urlSubdomainIndex = (m_urlSubdomainIndex + 1) % m_urlSubdomains.size();
    }

    UrlCallback onRequestFinish = [callback, task, url](UrlResponse&& response) {

        auto source = task->source();
        if (!source) {
            LOGW("URL Callback for deleted TileSource '%s'", url.string().c_str());
            return;
        }
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
