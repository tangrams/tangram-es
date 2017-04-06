#include "data/networkDataSource.h"

#include "log.h"
#include "platform.h"

namespace Tangram {

NetworkDataSource::NetworkDataSource(std::shared_ptr<Platform> _platform, const std::string& _urlTemplate,
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

    UrlCallback onRequestFinish = [this, callback, task, url](UrlResponse response) mutable {

        removePending(task->tileId(), false);

        if (task->isCanceled()) {
            return;
        }

        if (response.error) {
            LOGE("Error for URL request '%s': %s", url.string().c_str(), response.error);
            return;
        }

        if (!response.content.empty()) {
            auto& dlTask = static_cast<BinaryTileTask&>(*task);
            dlTask.rawTileData = std::make_shared<std::vector<char>>(std::move(response.content));
        }
        callback.func(task);
    };

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto requestHandle = m_platform->startUrlRequest(url, onRequestFinish);
        m_pending.push_back({ tileId, requestHandle });
    }

    return true;
}

void NetworkDataSource::removePending(const TileID& tile, bool cancelRequest) {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        if (it->tile == tile) {
            if (cancelRequest) {
                m_platform->cancelUrlRequest(it->request);
            }
            // This invalidates our iterators, so we return immediately.
            m_pending.erase(it);
            return;
        }
    }
}

void NetworkDataSource::cancelLoadingTile(const TileID& tile) {
    removePending(tile, true);
}

}
