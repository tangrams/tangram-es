#include "dataSource.h"
#include "util/geoJson.h"
#include "platform.h"
#include "tileData.h"
#include "tile/tileID.h"
#include "tile/tileHash.h"
#include "tile/tile.h"
#include "tile/tileManager.h"
#include "tile/tileTask.h"

#include <atomic>
#include <mutex>
#include <list>
#include <functional>
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
    int m_usage = 0;
    int m_maxUsage = 0;

    bool get(std::shared_ptr<TileTask>& _task) {

        if (m_maxUsage <= 0) { return false; }

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cacheMap.find(_task->tile->getID());
        if (it != m_cacheMap.end()) {
            // Move cached entry to start of list
            m_cacheList.splice(m_cacheList.begin(), m_cacheList, it->second);
            _task->rawTileData = m_cacheList.front().second;

            return true;
        }

        return false;
    }
    void put(const TileID& tileID, std::shared_ptr<std::vector<char>> rawDataRef) {

        if (m_maxUsage <= 0) { return; }

        std::lock_guard<std::mutex> lock(m_mutex);

        m_cacheList.push_front({tileID, rawDataRef});
        m_cacheMap[tileID] = m_cacheList.begin();

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

static std::atomic<int32_t> s_serial;

DataSource::DataSource(const std::string& _name, const std::string& _urlTemplate) :
    m_name(_name), m_urlTemplate(_urlTemplate),
    m_cache(std::make_unique<RawCache>()){

    m_id = s_serial++;
}

DataSource::~DataSource() {
    clearData();
}

void DataSource::setCacheSize(size_t _cacheSize) {
    m_cache->m_maxUsage = _cacheSize;
}

void DataSource::clearData() {
    m_cache->clear();
}

void DataSource::constructURL(const TileID& _tileCoord, std::string& _url) const {
    _url.assign(m_urlTemplate);
    try {
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

bool DataSource::getTileData(std::shared_ptr<TileTask>& _task) {
    if (m_cache->get(_task)) {
        _task->loaded = true;
        return true;
    }
    return false;
}

void DataSource::onTileLoaded(std::vector<char>&& _rawData, std::shared_ptr<TileTask>& _task, TileTaskCb _cb) {
    TileID tileID = _task->tile->getID();

    if (!_rawData.empty()) {
        _task->loaded = true;

        auto rawDataRef = std::make_shared<std::vector<char>>();
        std::swap(*rawDataRef, _rawData);
        _task->rawTileData = rawDataRef;

        _cb.func(std::move(_task));

        m_cache->put(tileID, rawDataRef);
    }
}


bool DataSource::loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    std::string url(constructURL(_task->tile->getID()));

    // Using bind instead of lambda to be able to 'move' (until c++14)
    return startUrlRequest(url, std::bind(&DataSource::onTileLoaded,
                                          this,
                                          std::placeholders::_1,
                                          std::move(_task), _cb));

}

void DataSource::cancelLoadingTile(const TileID& _tileID) {
    cancelUrlRequest(constructURL(_tileID));
}

}
