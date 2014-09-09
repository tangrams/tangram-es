#include "dataSource.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <cstdio>
#include <sstream>
#include <chrono>
#include <thread>
#include <sys/time.h>

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
    std::shared_ptr<std::string> tileID(new std::string(strStream.str()));
    return tileID;
}

//constructs a mapzen vectortile json url from the tile coordinates
static std::unique_ptr<std::string> constructURL(glm::ivec3 tileCoord) {
    std::ostringstream strStream;
    strStream<<"http://vector.mapzen.com/osm/all/"<<tileCoord.z
                <<"/"<<tileCoord.x<<"/"<<tileCoord.y<<".json";
    std::unique_ptr<std::string> url(new std::string(strStream.str()));
    return std::move(url);
}


//---- MapzenVectorTileJson Implementation----

// Responsible to read the tileData from the service
// takes a vector of tileCoordinates to be read from the service.
bool MapzenVectorTileJson::LoadTile(std::vector<glm::ivec3> tileCoords) {
    std::vector<std::shared_ptr<std::string>> tileIDs;
    std::vector<std::unique_ptr<std::string>> urls;

    //construct tileID and url for every tileCoord
    for(auto& tileCoord : tileCoords) {
        tileIDs.push_back(constructTileID(tileCoord));
        urls.push_back(constructURL(tileCoord));
    }

    CURLM *multiHandle;
    CURLMsg *handleMsg;
    // file descriptors to be used with curl_multi_fdset and select()
    fd_set fdRead;
    fd_set fdWrite;
    fd_set fdExcep;
    int fdMax;
    struct timeval timeout;
    int rc; //return value for select() call
    CURLMcode cres;

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
        //start fetching
        cres = curl_multi_perform(multiHandle, &numHandles);
        if(cres != CURLM_OK) {
            fprintf(stderr, "curl_multi_perform failed %d\n", cres);
            return false;
        }
        
        //if numHandles is 0, then multi_perform has no easy handles to perform fetching
        if(!numHandles) {
            std::cout<<"Warning: No URLS ready to read from";
            return true;
        }
        
        //Start fetching info untill no easy handle left to fetch data
        do {
            //set all file descriptors to 0
            FD_ZERO(&fdRead);
            FD_ZERO(&fdWrite);
            FD_ZERO(&fdExcep);

            //timeout specification for select() call
            //select() will unblock either when a fd is ready or tileout is reached
            timeout.tv_sec = 0;
            timeout.tv_usec = 100;

            //get file descriptors from the transfer
            cres = curl_multi_fdset(multiHandle, &fdRead, &fdWrite, &fdExcep, &fdMax);

            if(cres != CURLM_OK) {
                fprintf(stderr, "curl_multi_fdset failed %d\n", cres);
                return false;
            }

            while(fdMax < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                cres = curl_multi_perform(multiHandle, &numHandles);
                curl_multi_fdset(multiHandle, &fdRead, &fdWrite, &fdExcep, &fdMax);
                std::cout<<"fdMax: "<<fdMax<<" cres: "<<cres<<"\n";
            }

            rc = select(fdMax+1, &fdRead, &fdWrite, &fdExcep, &timeout);

            // see what select returned
            switch(rc) {
                case -1:
                    //ERROR
                    break;
                case 0:
                    break;
                default:
                    std::cout<<"Here: "<<numHandles<<"\n";
                    curl_multi_perform(multiHandle,&numHandles);
                    break;
            }
        }while(numHandles);
        
        curl_multi_cleanup(multiHandle);
        curl_global_cleanup();
    }

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
    return true;
}

//Returns jsonValue for a requested tileID
std::shared_ptr<Json::Value>
        MapzenVectorTileJson::GetData(std::string TileID) {
    return m_JsonRoots[TileID];
}
