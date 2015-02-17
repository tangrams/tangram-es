#include <curl/curl.h>

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

//write_data call back from CURLOPT_WRITEFUNCTION
//responsible to read and fill "stream" with the data.
static size_t write_data(void *_ptr, size_t _size, size_t _nmemb, void *_stream) {
    ((std::stringstream*) _stream)->write(reinterpret_cast<char *>(_ptr), _size * _nmemb);
    return _size * _nmemb;
}

NetworkDataSource::NetworkDataSource() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

NetworkDataSource::~NetworkDataSource() {
    curl_global_cleanup();
}

std::unique_ptr<std::string> NetworkDataSource::constructURL(const TileID& _tileCoord) {

    std::unique_ptr<std::string> urlPtr(new std::string(m_urlTemplate)); // Make a copy of our template

    size_t xpos = urlPtr->find("[x]");
    urlPtr->replace(xpos, 3, std::to_string(_tileCoord.x));
    
    size_t ypos = urlPtr->find("[y]");
    urlPtr->replace(ypos, 3, std::to_string(_tileCoord.y));
    
    size_t zpos = urlPtr->find("[z]");
    urlPtr->replace(zpos, 3, std::to_string(_tileCoord.z));
    
    if (xpos == std::string::npos || ypos == std::string::npos || zpos == std::string::npos) {
        logMsg("Bad URL template!!\n");
    }
    
    return std::move(urlPtr);
}

bool NetworkDataSource::loadTileData(const MapTile& _tile) {
    
    bool success = true; // Begin optimistically
    
    if (hasTileData(_tile.getID())) {
        // Tile has been fetched already!
        return success;
    }
    
    std::unique_ptr<std::string> url = constructURL(_tile.getID());

    CURL* curlHandle = curl_easy_init();

    // out will store the stringStream contents from curl
    std::stringstream out;
    
    // set up curl to perform fetch
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curlHandle, CURLOPT_URL, url->c_str());
    curl_easy_setopt(curlHandle, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "gzip");
    
    logMsg("Fetching URL with curl: %s\n", url->c_str());

    CURLcode result = curl_easy_perform(curlHandle);
    
    if (result != CURLE_OK) {
        
        logMsg("curl_easy_perform failed: %s\n", curl_easy_strerror(result));
        success = false;
        
    } else {
        
        // parse fetched data
        std::shared_ptr<TileData> tileData = parse(_tile, out);
        
        // Lock our mutex so that we can safely write to the tile store
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tileStore[_tile.getID()] = tileData;
        }
        
    }
    
    curl_easy_cleanup(curlHandle);
    
    return success;
}

//---- MapzenVectorTileJson Implementation----



