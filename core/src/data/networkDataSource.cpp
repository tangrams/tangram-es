#include "data/networkDataSource.h"

#include "log.h"
#include "platform.h"

#define MAX_DOWNLOADS 4

namespace Tangram {

NetworkDataSource::NetworkDataSource(std::shared_ptr<Platform> _platform, const std::string& _urlTemplate,
        std::vector<std::string>&& _urlSubdomains) :
    m_platform(_platform),
    m_urlTemplate(_urlTemplate),
    m_urlSubdomains(std::move(_urlSubdomains)),
    m_maxDownloads(MAX_DOWNLOADS) {}

void NetworkDataSource::constructURL(const TileID& _tileCoord, std::string& _url, int32_t index) const {
    _url.assign(m_urlTemplate);

    try {
        size_t subD = _url.find("{s}");
        if (subD != std::string::npos) {
            _url.replace(subD, 3, m_urlSubdomains[index]);
        }
        size_t xpos = _url.find("{x}");
        _url.replace(xpos, 3, std::to_string(_tileCoord.x));
        size_t ypos = _url.find("{y}");
        _url.replace(ypos, 3, std::to_string(_tileCoord.y));
        size_t zpos = _url.find("{z}");
        _url.replace(zpos, 3, std::to_string(_tileCoord.z));
    } catch(...) {
        LOGE("Bad URL template!");
    }
}

bool NetworkDataSource::loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) {

    if (_task->rawSource != this->level) {
        LOGE("NetworkDataSource must be last!");
        return false;
    }

    if (m_pending.size() >= m_maxDownloads) {
        return false;
    }

    auto tileId = _task->tileId();

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (std::find(m_pending.begin(), m_pending.end(), tileId) != m_pending.end()) {
            return false;
        }
        m_pending.push_back(tileId);
    }

    m_urlIndex = m_urlSubdomains.empty() ? 0 : (++m_urlIndex) % m_urlSubdomains.size();
    std::string url(constructURL(_task->tileId(), m_urlIndex));

    bool started = m_platform->startUrlRequest(url,
        [this, cb = _cb, task = _task](std::vector<char>&& _rawData) mutable {

            removePending(task->tileId());

            if (task->isCanceled()) {
                return;
            }

            if (!_rawData.empty()) {
                auto rawDataRef = std::make_shared<std::vector<char>>();
                std::swap(*rawDataRef, _rawData);

                auto& dlTask = static_cast<BinaryTileTask&>(*task);
                // NB: Sets hasData() state true
                dlTask.rawTileData = rawDataRef;
            }
            cb.func(task);
        });

    if (!started) {
        removePending(_task->tileId());

        // Set canceled state, so that tile will not be tried
        // for reloading until sourceGeneration increased.
        _task->cancel();
    }

    return started;
}

void NetworkDataSource::removePending(const TileID& _tileId) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto it = std::find(m_pending.begin(), m_pending.end(), _tileId);
    if (it != m_pending.end()) { m_pending.erase(it); }
}

void NetworkDataSource::cancelLoadingTile(const TileID& _tileId) {
    removePending(_tileId);
    // cancel all possible requests for this tile
    auto maxIndex = m_urlSubdomains.empty() ? 1 : m_urlSubdomains.size();
    for (size_t index = 0; index < maxIndex; index++) {
        m_platform->cancelUrlRequest(constructURL(_tileId, index));
    }
}

}
