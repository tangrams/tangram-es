#include "dataSource.h"
#include "util/geoJson.h"
#include "platform.h"
#include "tileData.h"
#include "tile/tileID.h"
#include "tile/tileHash.h"
#include "tile/tile.h"
#include "tile/tileManager.h"
#include "tile/tileTask.h"
#include "tile/mbtilesTileTask.h"
#include "util/mbtiles.h"
#include "gl/texture.h"
#include "log.h"

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

    bool get(DownloadTileTask& _task) {

        if (m_maxUsage <= 0) { return false; }

        std::lock_guard<std::mutex> lock(m_mutex);
        TileID id(_task.tileId().x, _task.tileId().y, _task.tileId().z);

        auto it = m_cacheMap.find(id);
        if (it != m_cacheMap.end()) {
            // Move cached entry to start of list
            m_cacheList.splice(m_cacheList.begin(), m_cacheList, it->second);
            _task.rawTileData = m_cacheList.front().second;
            _task.dataFromCache = true;
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

DataSource::DataSource(const std::string& _name, const std::string& _urlTemplate, const std::string& _mbtiles,
                       int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom) :
    m_name(_name), m_urlTemplate(_urlTemplate), m_mbtilesPath(_mbtiles),
    m_minDisplayZoom(_minDisplayZoom), m_maxDisplayZoom(_maxDisplayZoom), m_maxZoom(_maxZoom),
    m_cache(std::make_unique<RawCache>()){

    static std::atomic<int32_t> s_serial;

    m_id = s_serial++;
}

DataSource::~DataSource() {
    clearData();
}

std::shared_ptr<TileTask> DataSource::createTask(TileID _tileId, int _subTask) {
    std::shared_ptr<DownloadTileTask> task;
    if (hasMBTiles()) {
        task = std::make_shared<MBTilesTileTask>(_tileId, shared_from_this(), _subTask);
    } else {
        task = std::make_shared<DownloadTileTask>(_tileId, shared_from_this(), _subTask);
    }

    cacheGet(*task);

    return task;
}

void DataSource::setCacheSize(size_t _cacheSize) {
    m_cache->m_maxUsage = _cacheSize;
}

bool DataSource::cacheGet(DownloadTileTask& _task) {
    return m_cache->get(_task);
}

void DataSource::cachePut(const TileID& _tileID, std::shared_ptr<std::vector<char>> _rawDataRef) {
    m_cache->put(_tileID, _rawDataRef);
}

void DataSource::clearData() {
    m_cache->clear();
    m_generation++;
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

bool DataSource::equals(const DataSource& other) const {
    if (m_name != other.m_name) { return false; }
    if (m_urlTemplate != other.m_urlTemplate) { return false; }
    if (m_mbtilesPath != other.m_mbtilesPath) { return false; }
    if (m_minDisplayZoom != other.m_minDisplayZoom) { return false; }
    if (m_maxDisplayZoom != other.m_maxDisplayZoom) { return false; }
    if (m_maxZoom != other.m_maxZoom) { return false; }
    if (m_rasterSources.size() != other.m_rasterSources.size()) { return false; }
    for (size_t i = 0, end = m_rasterSources.size(); i < end; ++i) {
        if (!m_rasterSources[i]->equals(*other.m_rasterSources[i])) { return false; }
    }

    return true;
}

void DataSource::onTileLoaded(std::vector<char>&& _rawData, std::shared_ptr<TileTask>&& _task,
                              TileTaskCb _cb) {

    if (_task->isCanceled()) { return; }

    TileID tileID = _task->tileId();

    if (!_rawData.empty()) {

        auto rawDataRef = std::make_shared<std::vector<char>>();
        std::swap(*rawDataRef, _rawData);

        auto& task = static_cast<DownloadTileTask&>(*_task);
        task.rawTileData = rawDataRef;

        _cb.func(std::move(_task));

        cachePut(tileID, rawDataRef);
    }
}

bool DataSource::loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    /*
     * If our data source doesn't have a URL to make a request,
     * don't HTTP request data, we will fetch from MBTiles in a task.
     */
    if (hasNoUrl()) {
        _cb.func(std::move(_task));
        return true;
    }
    /*
     * If we have a URL, we should request a tile.
     * We'll later cache it in MBTiles and / or memory
     * in a task.
     */
    else {
        std::string url(constructURL(_task->tileId()));

        // lambda captured parameters are const by default, we want "task" (moved) to be non-const,
        // hence "mutable"
        // Refer: http://en.cppreference.com/w/cpp/language/lambda
        return startUrlRequest(url,
                               [this, _cb, task = std::move(_task)](std::vector<char>&& rawData) mutable {
                                   this->onTileLoaded(std::move(rawData), std::move(task), _cb);
                               });
    }
}

void DataSource::cancelLoadingTile(const TileID& _tileID) {
    cancelUrlRequest(constructURL(_tileID));
    for (auto& raster : m_rasterSources) {
        TileID rasterID = _tileID.withMaxSourceZoom(raster->maxZoom());
        raster->cancelLoadingTile(rasterID);
    }
}

void DataSource::clearRasters() {
    for (auto& raster : m_rasterSources) {
        raster->clearRasters();
    }
}

void DataSource::clearRaster(const TileID& id) {
    for (auto& raster : m_rasterSources) {
        TileID rasterID = id.withMaxSourceZoom(raster->maxZoom());
        raster->clearRaster(rasterID);
    }
}

void DataSource::addRasterSource(std::shared_ptr<DataSource> _rasterSource) {
    /*
     * We limit the parent source by any attached raster source's min/max.
     */
    int32_t rasterMinDisplayZoom = _rasterSource->minDisplayZoom();
    int32_t rasterMaxDisplayZoom = _rasterSource->maxDisplayZoom();
    if (rasterMinDisplayZoom > m_minDisplayZoom) {
        m_minDisplayZoom = rasterMinDisplayZoom;
    }
    if (rasterMaxDisplayZoom < m_maxDisplayZoom) {
        m_maxDisplayZoom = rasterMaxDisplayZoom;
    }
    m_rasterSources.push_back(_rasterSource);
}

void DataSource::setupMBTiles() {
    // If we have a path to an MBTiles file,
    // try to open up a SQLite database instance.
    if (m_mbtilesPath.size() > 0) {
        try {
            // Need to explicitly open a SQLite DB with OPEN_READWRITE and OPEN_CREATE flags to make a file and write.
            m_mbtilesDb = std::make_unique<SQLite::Database>(m_mbtilesPath, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
            LOG("MBTiles SQLite DB Opened at: %s", m_mbtilesPath.c_str());

            // If needed, setup the database by running the schema.sql.
            MBTiles::setupDB(*this);

        } catch (std::exception& e) {
            LOGE("Unable to open SQLite database: %s", e.what());
        }
    }
}

}
