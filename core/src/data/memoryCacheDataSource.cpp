#include "data/memoryCacheDataSource.h"

#include "tile/tileHash.h"
#include "tile/tileID.h"
#include "log.h"

#include <list>
#include <mutex>
#include <unordered_map>

namespace Tangram {

struct RawCache {

    // Used to ensure safe access from async loading threads
    std::mutex m_mutex;

    // LRU in-memory cache for raw tile data
    using CacheEntry = std::pair<TileID, std::shared_ptr<std::vector<char>>>;
    using CacheList = std::list<CacheEntry>;
    using CacheMap = std::unordered_map<TileID, typename CacheList::iterator>;

    CacheMap m_cacheMap;
    CacheList m_cacheList;
    size_t m_usage = 0;
    size_t m_maxUsage = 0;

    bool has(const TileID& _tileID) {

        if (m_maxUsage <= 0) { return false; }

        std::lock_guard<std::mutex> lock(m_mutex);

        return m_cacheMap.find(_tileID) != m_cacheMap.end();
    }

    bool get(BinaryTileTask& _task) {

        if (m_maxUsage <= 0) { return false; }

        std::lock_guard<std::mutex> lock(m_mutex);
        const auto& taskTileID = _task.tileId();
        TileID id(taskTileID.x, taskTileID.y, taskTileID.z);

        auto it = m_cacheMap.find(id);
        if (it != m_cacheMap.end()) {
            // Move cached entry to start of list
            m_cacheList.splice(m_cacheList.begin(), m_cacheList, it->second);
            _task.rawTileData = m_cacheList.front().second;

            return true;
        }

        return false;
    }
    void put(const TileID& tileID, std::shared_ptr<std::vector<char>> rawDataRef) {

        if (m_maxUsage <= 0) { return; }

        std::lock_guard<std::mutex> lock(m_mutex);
        TileID id(tileID.x, tileID.y, tileID.z);

        m_cacheList.push_front({id, rawDataRef});
        m_cacheMap[id] = m_cacheList.begin();

        m_usage += rawDataRef->size();

        while (m_usage > m_maxUsage) {
            if (m_cacheList.empty()) {
                LOGE("Error: invalid cache state!");
                m_usage = 0;
                break;
            }

            // LOGE("Limit raw cache tiles:%d, %fMB ", m_cacheList.size(),
            //        double(m_cacheUsage) / (1024*1024));

            auto& entry = m_cacheList.back();
            m_usage -= entry.second->size();

            m_cacheMap.erase(entry.first);
            m_cacheList.pop_back();
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cacheMap.clear();
        m_cacheList.clear();
        m_usage = 0;
    }
};


MemoryCacheDataSource::MemoryCacheDataSource() :
    m_cache(std::make_unique<RawCache>()) {
}

MemoryCacheDataSource::~MemoryCacheDataSource() {}

void MemoryCacheDataSource::setCacheSize(size_t _cacheSize) {
    m_cache->m_maxUsage = _cacheSize;
}

bool MemoryCacheDataSource::hasCache(const TileID& _tileID) {
    return m_cache->has(_tileID);
}

bool MemoryCacheDataSource::cacheGet(BinaryTileTask& _task) {
    return m_cache->get(_task);
}

void MemoryCacheDataSource::cachePut(const TileID& _tileID, std::shared_ptr<std::vector<char>> _rawDataRef) {
    m_cache->put(_tileID, _rawDataRef);
}

TileID MemoryCacheDataSource::getFallbackTileID(const TileID& _tileID, int32_t _maxZoom, int32_t _zoomBias) {

    if (hasCache(_tileID.zoomBiasAdjusted(_zoomBias).withMaxSourceZoom(_maxZoom))) {
        if (next) {
            TileID nextTileID = next->getFallbackTileID(_tileID, _maxZoom, _zoomBias);
            return (_tileID.z > nextTileID.z) ? _tileID : nextTileID;
        }

        return _tileID;
    }

    TileID tileID(_tileID);
    bool isCached = false;

    if (_zoomBias > 0) {
        while (!(isCached = hasCache(tileID.zoomBiasAdjusted(_zoomBias))) && tileID.z > _zoomBias) {
            tileID = tileID.zoomBiasAdjusted(_zoomBias);
        }
    }
    else {
        while (!(isCached = hasCache(tileID)) && tileID.z > 0) {
            tileID = tileID.getParent(_zoomBias);
        }
    }

    if (next) {
        TileID nextTileID = next->getFallbackTileID(_tileID, _maxZoom, _zoomBias);

        if (isCached) {
            return (tileID.z > nextTileID.z) ? tileID : nextTileID;
        }
        else {
            return nextTileID;
        }
    }

    return _tileID;
}

bool MemoryCacheDataSource::loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) {

    auto& task = static_cast<BinaryTileTask&>(*_task);

    if (_task->rawSource == this->level) {

        cacheGet(task);

        if (task.hasData()) {
            _cb.func(_task);
            return true;
        }

        // Try next source on subsequent calls
        if (next) { _task->rawSource = next->level; }
    }

    if (next) {

        return next->loadTileData(_task, {[this, _cb](std::shared_ptr<TileTask> _task) {

            auto& task = static_cast<BinaryTileTask&>(*_task);

            if (task.hasData()) { cachePut(task.tileId(), task.rawTileData); }

            _cb.func(_task);
        }});
    }

    return false;
}

void MemoryCacheDataSource::clear() {
    m_cache->clear();

    if (next) { next->clear(); }
}

}
