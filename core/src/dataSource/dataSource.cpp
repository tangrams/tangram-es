#include <sstream>
#include <cstdio>

#include <curl/curl.h>

#include "dataSource.h"

void DataSource::clearData() {
    for (auto& mapValue : m_JsonRoots) {
        mapValue.second->clear();
    }
    m_JsonRoots.clear();
}

//----Curl Helper Functions----

//write_data call back from CURLOPT_WRITEFUNCTION
//responsible to read and fill "stream" with the data.
static size_t write_data(char *_ptr, size_t _size, size_t _nmemb, void *_stream) {
    ((std::stringstream*) _stream)->write(_ptr, _size * _nmemb);
    return _size * _nmemb;
}

//---- MapzenVectorTileJson Implementation----

MapzenVectorTileJson::MapzenVectorTileJson() {
    m_urlTemplate = "http://vector.mapzen.com/osm/all/[z]/[x]/[y].json";
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

std::unique_ptr<std::string> MapzenVectorTileJson::constructURL(const TileID& _tileCoord) {

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

bool MapzenVectorTileJson::loadTile(const TileID& _tileID) {
    
    bool success = true; // Begin optimistically
    
    if (hasTileData(_tileID)) {
        // Tile has been fetched already!
        return success;
    }
    
    std::unique_ptr<std::string> url = constructURL(_tileID);

    CURL* curlHandle = curl_easy_init();

    // out will store the stringStream contents from curl
    std::stringstream out;
    
    // set up curl to perform fetch
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curlHandle, CURLOPT_URL, url->c_str());
    curl_easy_setopt(curlHandle, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0L);
    
    logMsg("Fetching URL with curl: %s\n", url->c_str());

    CURLcode result = curl_easy_perform(curlHandle);
    
    if (result != CURLE_OK) {
        
        logMsg("curl_easy_perform failed: %s\n", curl_easy_strerror(result));
        success = false;
        
    } else {
        
        // parse written data into a JSON object
        Json::Reader jsonReader;
        std::shared_ptr<Json::Value> jsonValue = std::make_shared<Json::Value>();
        
        if (jsonReader.parse(out, *jsonValue)) {
            
            m_JsonRoots[_tileID] = jsonValue;
            
        } else {
            
            logMsg("Json parsing failed on tile %s\n", url->c_str());
            success = false;
            
        }
    }
    
    curl_easy_cleanup(curlHandle);
    
    return success;
}

bool MapzenVectorTileJson::hasTileData(const TileID& _tileID) {
    
    return m_JsonRoots.find(_tileID) != m_JsonRoots.end();
    
}

std::shared_ptr<Json::Value> MapzenVectorTileJson::getTileData(const TileID& _tileID) {
    
    return hasTileData(_tileID) ? m_JsonRoots[_tileID] : nullptr;
    
}

MapzenVectorTileJson::~MapzenVectorTileJson() {
    curl_global_cleanup();
}

