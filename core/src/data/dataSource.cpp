#include "dataSource.h"
#include "util/geoJson.h"
#include "platform.h"
#include "tileData.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileManager.h"

#include <atomic>

namespace Tangram {

static std::atomic<int32_t> s_serial;

DataSource::DataSource(const std::string& _name, const std::string& _urlTemplate) :
    m_name(_name), m_urlTemplate(_urlTemplate),
    m_cacheUsage(0),
    m_cacheMaxUsage(0) {

    m_id = s_serial++;
}

DataSource::~DataSource() {
    clearData();
}

void DataSource::setCacheSize(size_t _cacheSize) {
    m_cacheMaxUsage = _cacheSize;
}

void DataSource::clearData() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cacheMap.clear();
    m_cacheList.clear();
    m_cacheUsage = 0;
}

void DataSource::constructURL(const TileID& _tileCoord, std::string& _url) const {
    _url.assign(m_urlTemplate);

    size_t xpos = _url.find("{x}");
    _url.replace(xpos, 3, std::to_string(_tileCoord.x));

    size_t ypos = _url.find("{y}");
    _url.replace(ypos, 3, std::to_string(_tileCoord.y));

    size_t zpos = _url.find("{z}");
    _url.replace(zpos, 3, std::to_string(_tileCoord.z));

    if (xpos == std::string::npos || ypos == std::string::npos || zpos == std::string::npos) {
        logMsg("Bad URL template!!\n");
    }
}

bool DataSource::getTileData(std::shared_ptr<TileTask>& _task) {
    if (m_cacheMaxUsage > 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cacheMap.find(_task->tile->getID());
        if (it != m_cacheMap.end()) {
            // Move cached entry to start of list
            m_cacheList.splice(m_cacheList.begin(), m_cacheList, it->second);
            _task->rawTileData = m_cacheList.front().second;

            return true;
        }
    }

    return false;
}

void DataSource::onTileLoaded(std::vector<char>&& _rawData, std::shared_ptr<TileTask>& _task, TileTaskCb _cb) {
    TileID tileID = _task->tile->getID();

    auto rawDataRef = std::make_shared<std::vector<char>>();
    std::swap(*rawDataRef, _rawData);
    _task->rawTileData = rawDataRef;

    _cb(std::move(_task));

    if (m_cacheMaxUsage > 0) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_cacheList.push_front({tileID, rawDataRef});
        m_cacheMap[tileID] = m_cacheList.begin();

        m_cacheUsage += rawDataRef->size();

        while (m_cacheUsage > m_cacheMaxUsage) {
            if (m_cacheList.empty()) {
                logMsg("Error: invalid cache state!\n");
                m_cacheUsage = 0;
                break;
            }

            // logMsg("Limit raw cache tiles:%d, %fMB \n", m_cacheList.size(),
            //        double(m_cacheUsage) / (1024*1024));

            auto& entry = m_cacheList.back();
            m_cacheUsage -= entry.second->size();

            m_cacheMap.erase(entry.first);
            m_cacheList.pop_back();
        }
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
