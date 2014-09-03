#include "dataSource.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <cstdio>
#include <sstream>

#include <curl/multi.h>


void DataSource::ClearGeoRoots() {
    for (auto& mapValue : m_JsonRoots) {
        mapValue.second->clear();
    }
    m_JsonRoots.clear();
}


//--Curl Helper Functions
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    std::string data((const char*) ptr, (size_t) size*nmemb);
    *((std::stringstream*) stream) << data;
    return size*nmemb;
}

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

static std::shared_ptr<std::string> constructTileID(glm::vec3 tileCoord) {
    std::ostringstream strStream;
    strStream<<tileCoord.x<<"_"<<tileCoord.y<<"_"<<tileCoord.z;
    std::cout<<strStream.str();
    std::shared_ptr<std::string> tileID(new std::string(strStream.str()));
    return tileID;
}

static std::unique_ptr<std::string> constructURL(glm::vec3 tileCoord) {
    std::ostringstream strStream;
    strStream<<"http://vector.mapzen.com/osm/all/"<<tileCoord.z
                <<"/"<<tileCoord.x<<"/"<<tileCoord.y<<".json";
    std::cout<<strStream.str();
    std::unique_ptr<std::string> url(new std::string(strStream.str()));
    return std::move(url);
}


//--- MapzenVectorTileJson Implementation
void MapzenVectorTileJson::LoadTile(std::vector<glm::vec3> tileCoords) {
    /*std::string tileIDs[3];
    tileIDs[0] = "0_0_0";
    tileIDs[1] = "16_19293_24641";
    tileIDs[2] = "14_19293_24641";
    std::vector<std::string> testURLs;
    testURLs.push_back("http://vector.mapzen.com/osm/all/0/0/0.json");
    testURLs.push_back("http://vector.mapzen.com/osm/all/16/19293/24641.json");
    testURLs.push_back("http://vector.mapzen.com/osm/all/14/19293/24641.json");*/
    std::vector<std::shared_ptr<std::string>> tileIDs;
    std::vector<std::unique_ptr<std::string>> urls;
    for(auto& tileCoord : tileCoords) {
        tileIDs.push_back(constructTileID(tileCoord));
        urls.push_back(constructURL(tileCoord));
    }

    CURLM *multiHandle;
    CURLMsg *handleMsg;
    int queuedHandles, numHandles = urls.size();
    std::stringstream *out[urls.size()];

    curl_global_init(CURL_GLOBAL_DEFAULT);

    multiHandle = curl_multi_init();
    int count = 0;
    for(auto& url : urls) {
        out[count] = new std::stringstream;
        curlInit(multiHandle, *url.get(), out[count]);
        count++;
    }
    /*curlInit(multiHandle, testURLs[0], &out[0]);
    curlInit(multiHandle, testURLs[1], &out[1]);
    curlInit(multiHandle, testURLs[2], &out[2]);*/
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
/*std::vector<glm::vec3> MapzenVectorTileJson::LoadTile() {

    // tildID and FileName is explicitly hard coded, these
    // will be read from the service eventually using libCurl

    std::vector<glm::vec3> tileIDs;
    tileIDs.push_back(glm::vec3(1, 2, 3));
    //Need to do this to have a std::map, which respects only
    std::string tileID("1_2_3");
    std::string FileName("/Users/Varun/Development/tangram-es/data/test.json");

    // Json Extracting
    // TODO: Try Janson Library or rapidJson library
    std::shared_ptr<Json::Value> jsonLocalValue(new Json::Value);
    std::fstream inputStream(FileName, std::fstream::in);
    inputStream.seekg (0, inputStream.end);
    int length = inputStream.tellg();
    inputStream.seekg (0, inputStream.beg);
    char *jsonFileBuffer = new char[length];
    inputStream.read(jsonFileBuffer, length);
    Json::Reader jsonReader;
    jsonReader.parse(jsonFileBuffer, jsonFileBuffer + length, *(jsonLocalValue.get()));
    m_JsonRoots[tileID] = jsonLocalValue;
    return std::move(tileIDs);
}*/

std::shared_ptr<Json::Value>
        MapzenVectorTileJson::GetData(std::string TileID) {
    return m_JsonRoots[TileID];
}
