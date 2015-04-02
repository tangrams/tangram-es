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

//---- NetworkDataSource Implementation----

NetworkDataSource::NetworkDataSource() {
}

NetworkDataSource::~NetworkDataSource() {
}

void NetworkDataSource::constructURL(const TileID& _tileCoord, std::string& _url) {

    _url.assign(m_urlTemplate);

    size_t xpos = _url.find("[x]");
    _url.replace(xpos, 3, std::to_string(_tileCoord.x));
    
    size_t ypos = _url.find("[y]");
    _url.replace(ypos, 3, std::to_string(_tileCoord.y));
    
    size_t zpos = _url.find("[z]");
    _url.replace(zpos, 3, std::to_string(_tileCoord.z));
    
    if (xpos == std::string::npos || ypos == std::string::npos || zpos == std::string::npos) {
        logMsg("Bad URL template!!\n");
    }
}

bool NetworkDataSource::loadTileData(const MapTile& _tile) {
    
    bool success = true; // Begin optimistically
    
    if (hasTileData(_tile.getID())) {
        // Tile has been fetched already!
        return success;
    }

    std::string url;
    
    constructURL(_tile.getID(), url);
    
    std::stringstream rawData;

    success = streamFromHttpSync(url, rawData);

    if(rawData) {

        // parse fetched data
        std::shared_ptr<TileData> tileData = parse(_tile, rawData);
        
        // Lock our mutex so that we can safely write to the tile store
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tileStore[_tile.getID()] = tileData;
        }

    } else {
        success = false;
    }
    
    
    return success;
}

