#include "dataSource.h"
#include "util/geoJson.h"
#include "platform.h"
#include "tileData.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileManager.h"
#include "labels/labels.h"

namespace Tangram {

DataSource::DataSource(const std::string& _name, const std::string& _urlTemplate) :
    m_name(_name), m_urlTemplate(_urlTemplate) {

}

bool DataSource::hasTileData(const TileID& _tileID) const {

    return m_tileStore.find(_tileID) != m_tileStore.end();
}

std::shared_ptr<TileData> DataSource::getTileData(const TileID& _tileID) const {

    const auto it = m_tileStore.find(_tileID);

    if (it != m_tileStore.end()) {
        std::shared_ptr<TileData> tileData = it->second;
        return tileData;
    } else {
        return nullptr;
    }
}

void DataSource::clearData() {
    for (auto& mapValue : m_tileStore) {
        mapValue.second->layers.clear();
    }
    m_tileStore.clear();
}

void DataSource::setTileData(const TileID& _tileID, const std::shared_ptr<TileData>& _tileData) {

        std::lock_guard<std::mutex> lock(m_mutex);
        m_tileStore[_tileID] = _tileData;
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


bool DataSource::getTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    const auto& tileID = _task->tile->getID();

    if (hasTileData(tileID)) {
        _task->parsedTileData = m_tileStore[tileID];
        _cb(std::move(_task));
        requestRender();

        return true;
    }

    return false;
}

static void onTileLoaded(std::vector<char>&& _rawData, std::shared_ptr<TileTask>& _task, TileTaskCb _cb) {
    _task->rawTileData = std::move(_rawData);
    _cb(std::move(_task));
    requestRender();
}


bool DataSource::loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    std::string url(constructURL(_task->tile->getID()));

    return startUrlRequest(url, std::bind(&onTileLoaded,
                                          std::placeholders::_1,
                                          std::move(_task), _cb));

}

void DataSource::cancelLoadingTile(const TileID& _tileID) {
    cancelUrlRequest(constructURL(_tileID));
}

}
