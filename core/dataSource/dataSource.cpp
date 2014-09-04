#include "dataSource.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <cstdio>
#include <sstream>

#include <curl/multi.h>

// clears the tileID:jsonRoot map.
void DataSource::ClearGeoRoots() {
    for (auto& mapValue : m_JsonRoots) {
        mapValue.second->clear();
    }
    m_JsonRoots.clear();
}


//----Curl Helper Functions----

//write_data call back from CURLOPT_WRITEFUNCTION
//responsible to read and fill "stream" with the data.
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    std::string data((const char*) ptr, (size_t) size*nmemb);
    *((std::stringstream*) stream) << data;
    return size*nmemb;
}

// curlInit initializes individual curl simple instances
// every tile url has a curl simple instance
static void curlInit(CURLM *curlMulti, std::string url, std::stringstream *out) {
    CURL *curlEasy = curl_easy_init();
    curl_easy_setopt(curlEasy, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlEasy, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curlEasy, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curlEasy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlEasy, CURLOPT_PRIVATE, url.c_str());
    curl_easy_setopt(curlEasy, CURLOPT_VERBOSE, 0L);

    curl_multi_add_handle(curlMulti, curlEasy);
    return;
}

//---- tileID and url construction----

//constructs a string from the tile coodinates
static std::shared_ptr<std::string> constructTileID(glm::ivec3 tileCoord) {
    std::ostringstream strStream;
    strStream<<tileCoord.x<<"_"<<tileCoord.y<<"_"<<tileCoord.z;
    std::cout<<strStream.str();
    std::shared_ptr<std::string> tileID(new std::string(strStream.str()));
    return tileID;
}

//constructs a mapzen vectortile json url from the tile coordinates
static std::unique_ptr<std::string> constructURL(glm::ivec3 tileCoord) {
    std::ostringstream strStream;
    strStream<<"http://vector.mapzen.com/osm/all/"<<tileCoord.z
                <<"/"<<tileCoord.x<<"/"<<tileCoord.y<<".json";
    std::cout<<strStream.str();
    std::unique_ptr<std::string> url(new std::string(strStream.str()));
    return std::move(url);
}


//---- MapzenVectorTileJson Implementation----

// Responsible to read the tileData from the service
// takes a vector of tileCoordinates to be read from the service.
void MapzenVectorTileJson::LoadTile(std::vector<glm::ivec3> tileCoords) {
    std::vector<std::shared_ptr<std::string>> tileIDs;
    std::vector<std::unique_ptr<std::string>> urls;

    //construct tileID and url for every tileCoord
    for(auto& tileCoord : tileCoords) {
        tileIDs.push_back(constructTileID(tileCoord));
        urls.push_back(constructURL(tileCoord));
    }

    CURLM *multiHandle;
    CURLMsg *handleMsg;
    int queuedHandles, numHandles = urls.size();
    // out will store the stringStream contents from libCurl
    std::stringstream *out[urls.size()];

    curl_global_init(CURL_GLOBAL_DEFAULT);

    multiHandle = curl_multi_init();
    int count = 0;

    // initialize curl simple interface for every url
    for(auto& url : urls) {
        out[count] = new std::stringstream;
        curlInit(multiHandle, *url.get(), out[count]);
        count++;
    }

    //do curl stuff
    if(multiHandle) {
        while(numHandles) {
            curl_multi_perform(multiHandle, &numHandles);
            while ((handleMsg = curl_multi_info_read(
                                multiHandle, &queuedHandles))) {
                if (handleMsg->msg == CURLMSG_DONE) {
                    char *url;
                    CURL *e = handleMsg->easy_handle;
                    curl_easy_getinfo(handleMsg->easy_handle, CURLINFO_PRIVATE, &url);
                    fprintf(stderr, "R: %d - %s <%s>\n",
                            handleMsg->data.result, curl_easy_strerror(
                                handleMsg->data.result), url);
                    curl_multi_remove_handle(multiHandle, e);
                    curl_easy_cleanup(e);
                }
                else {
                    fprintf(stderr, "E: CURLMsg (%d)\n", handleMsg->msg);
                }
            }
        }
    }
    curl_multi_cleanup(multiHandle);
    curl_global_cleanup();

    // set map data (tileID->JsonValue) for every url
    for(auto i = 0; i < urls.size(); i++) {
        std::shared_ptr<Json::Value> jsonVal(new Json::Value);
        std::string tmp = out[i]->str();
        int length = tmp.size();
        Json::Reader jsonReader;
        jsonReader.parse(tmp.c_str(), tmp.c_str() + length, *(jsonVal.get()));
        m_JsonRoots[*(tileIDs.at(i).get())] = jsonVal;
        delete out[i];
    }
    tileIDs.clear();
    urls.clear();
}

//Returns jsonValue for a requested tileID
std::shared_ptr<Json::Value>
        MapzenVectorTileJson::GetData(std::string TileID) {
    return m_JsonRoots[TileID];
}
