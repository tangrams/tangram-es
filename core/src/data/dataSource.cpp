#include "geoJson.h"
#include "dataSource.h"
#include "platform.h"
#include "tileID.h"
#include "tileData.h"
#include "mapTile.h"

//---- DataSource Implementation----

bool DataSource::hasTileData(const TileID& _tileID) {
    
    return m_tileStore.find(_tileID) != m_tileStore.end();
}

std::shared_ptr<TileData> DataSource::getTileData(const TileID& _tileID) {
    
    if (hasTileData(_tileID)) {
        std::shared_ptr<TileData> tileData = m_tileStore[_tileID];
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

void DataSource::setUrlTemplate(const std::string& _urlTemplate){
    m_urlTemplate = _urlTemplate;
}

//---- NetworkDataSource Implementation----

NetworkDataSource::NetworkDataSource() {
}

NetworkDataSource::~NetworkDataSource() {
}

void NetworkDataSource::constructURL(const TileID& _tileCoord, std::string& _url) {

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

bool NetworkDataSource::loadTileData(const TileID& _tileID, const int _dataSourceID) {
    
    bool success = true; // Begin optimistically
    
    if (hasTileData(_tileID)){
        // Tile has been fetched already!
        return success;
    }

    std::string url;
    
    constructURL(_tileID, url);

    success = startNetworkRequest(url, _tileID, _dataSourceID);
    
    return success;
}

void NetworkDataSource::cancelLoadingTile(const TileID& _tileID) {
    std::string url;
    constructURL(_tileID, url);
    cancelNetworkRequest(url);
}

