#include "dataSource.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <cstdio>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <sys/time.h>

#include <curl/multi.h>

static const int MAX_TRY = 3;

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
size_t write_data(void *_ptr, size_t _size, size_t _nmemb, void *_stream) {
    std::string data((const char*) _ptr, (size_t) _size*_nmemb);
    *((std::stringstream*) _stream) << data;
    return _size*_nmemb;
}

// curlInit initializes individual curl simple instances
// every tile url has a curl simple instance
static void curlInit(CURLM *_curlMulti, std::string _url, std::stringstream *_out) {
    CURL *curlEasy = curl_easy_init();
    curl_easy_setopt(curlEasy, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlEasy, CURLOPT_WRITEDATA, _out);
    curl_easy_setopt(curlEasy, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curlEasy, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(curlEasy, CURLOPT_PRIVATE, (char *)_out);
    curl_easy_setopt(curlEasy, CURLOPT_VERBOSE, 0L);

    curl_multi_add_handle(_curlMulti, curlEasy);
    return;
}

//---- tileID and url construction----

//constructs a string from the tile coodinates
//TODO: Use regex to do this better.
static std::shared_ptr<std::string> constructTileID(glm::ivec3 _tileCoord) {
    std::ostringstream strStream;
    strStream<<_tileCoord.x<<"_"<<_tileCoord.y<<"_"<<_tileCoord.z;
    std::shared_ptr<std::string> tileID(new std::string(strStream.str()));
    return tileID;
}

//constructs a mapzen vectortile json url from the tile coordinates
//TODO: Use regex to do this better.
static std::unique_ptr<std::string> constructURL(glm::ivec3 _tileCoord) {
    std::ostringstream strStream;
    strStream<<"http://vector.mapzen.com/osm/all/"<<_tileCoord.z
                <<"/"<<_tileCoord.x<<"/"<<_tileCoord.y<<".json";
    std::unique_ptr<std::string> url(new std::string(strStream.str()));
    return std::move(url);
}

//TODO: Use regex to do this better.
// Hacking to extract id from url
static std::string extractIDFromUrl(std::string _url) {
    int x,y,z;
    std::string baseURL("http://vector.mapzen.com/osm/all/");
    std::string jsonStr(".json");
    std::string tmpID = _url.replace(0, baseURL.length(), "");
    std::size_t jsonPos = tmpID.find(jsonStr);
    tmpID = tmpID.replace(jsonPos, jsonStr.length(), "");
    std::replace(tmpID.begin(), tmpID.end(), '/','_');
    return tmpID;
}


//---- MapzenVectorTileJson Implementation----

// Responsible to read the tileData from the service
// takes a vector of tileCoordinates to be read from the service.
bool MapzenVectorTileJson::LoadTile(std::vector<glm::ivec3> _tileCoords) {
    std::vector<std::shared_ptr<std::string>> tileIDs;
    std::vector<std::unique_ptr<std::string>> urls;

    //construct tileID and url for every tileCoord
    for(auto& tileCoord : _tileCoords) {
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
    int prevHandle;
    int try = 0; //Counter to check for curl/select timeOuts.. maxed by static count MAX_TRY
    
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
        
        //Start fetching info until no easy handle left to fetch data
        do {
            //set all file descriptors to 0
            FD_ZERO(&fdRead);
            FD_ZERO(&fdWrite);
            FD_ZERO(&fdExcep);

            //timeout specification for select() call
            //select() will unblock either when a fd is ready or tileout is reached
            timeout.tv_sec = 1; //enough time for fd to be ready reading data... could be optimized.
            timeout.tv_usec = 0;

            //get file descriptors from the transfer
            cres = curl_multi_fdset(multiHandle, &fdRead, &fdWrite, &fdExcep, &fdMax);

            if(cres != CURLM_OK) {
                fprintf(stderr, "curl_multi_fdset failed %d\n", cres);
                return false;
            }
            
            //wait and repeat until curl has something to report to the kernel wrt file descriptors
            while(fdMax < 0) {
                //TODO: Get a better heuristic on the sleep milliseconds

                //sleeps for 100 msec and calls perform and fdset to see if multi perform has started its job
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                cres = curl_multi_perform(multiHandle, &numHandles);
                prevHandle = numHandles;
                curl_multi_fdset(multiHandle, &fdRead, &fdWrite, &fdExcep, &fdMax);
                std::cout<<"Here\n"; //TODO: Remove this. Its here to test how many times this loop runs till
                                     //multi_perform starts doing stuff
            }

            //select blocks the thread until the fd set by curl is ready with data.
            rc = select(fdMax+1, &fdRead, &fdWrite, &fdExcep, &timeout);

            // helper variables to convert extracted data to Json on the spot instead of waiting for all urls to be
            // fetched and then converting the extracted data to json
            char *url;
            char *tmpOutData; //to read the CURLINFO_PRIVATE data which is type casted to char* from stringstream*
            std::string tmpJsonData;
            int length;
            std::shared_ptr<Json::Value> jsonVal(new Json::Value);
            Json::Reader jsonReader;

            // see what select returned
            switch(rc) {
                case -1:
                    //select call ERRORed
                    break;
                case 0:
                    std::cout<<"Here timeout\n"; //TODO: Remove this. Its here to test how many times select times out.
                                                 // So far never with 1 sec of timeout.
                    //select call Timed out. No fd ready to read anything.
                    try++;
                    if(try == MAX_TRY) {
                        curl_multi_cleanup(multiHandle);
                        curl_global_cleanup();
                        for(auto i = 0; i < urls.size(); i++) {
                            delete out[i];
                        }
                        tileIDs.clear();
                        urls.clear();
                        return false;
                    }
                    break;
                default:
                    // sleep for 5 msec to give enough time for curl to read data for any of the file descriptors.
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    std::cout<<"Possible Change\n"; //TODO: Remove this. Its here to test how many times fd is ready and
                                                    // will result in a complete data read
                    //Perform again to see what happened with individual easy handles
                    curl_multi_perform(multiHandle,&numHandles);
                    // if easy  handle status changed some urls are done.
                    if(prevHandle != numHandles) {
                        std::cout<<"Change happened\n";
                        prevHandle = numHandles;
                        handleMsg = curl_multi_info_read(multiHandle, &queuedHandles);
                        // for every url done fill the jsonValue
                        for(auto qHandItr = 0; qHandItr < queuedHandles; qHandItr++) {
                            if(handleMsg->msg == CURLMSG_DONE) {
                                //get the url from the easyHandle
                                curl_easy_getinfo(handleMsg->easy_handle, CURLINFO_EFFECTIVE_URL , &url);
                                //get the tmpOutData which is holding the extracted info from the url
                                curl_easy_getinfo(handleMsg->easy_handle, CURLINFO_PRIVATE , &tmpOutData);
                                // typecast back from char* to std::stringstream
                                tmpJsonData = ((std::stringstream *)tmpOutData)->str();
                                length = tmpJsonData.size();
                                jsonReader.parse(tmpJsonData.c_str(), tmpJsonData.c_str() + length, *(jsonVal.get()));
                                // no way to get what ID this url was for so have to extract ID from url
                                m_JsonRoots[extractIDFromUrl(std::string(url))] = jsonVal;
                                fprintf(stderr, "R: %d - %s <%s>\n", handleMsg->data.result,
                                           curl_easy_strerror(handleMsg->data.result), url);
                                curl_multi_remove_handle(multiHandle, handleMsg->easy_handle);
                                curl_easy_cleanup(handleMsg->easy_handle);
                            }
                        }
                    }
                    break;
            }
        }while(numHandles);
        
        curl_multi_cleanup(multiHandle);
        curl_global_cleanup();
    }

    // set map data (tileID->JsonValue) for every url
    /*for(auto i = 0; i < urls.size(); i++) {
        std::shared_ptr<Json::Value> jsonVal(new Json::Value);
        std::string tmp = out[i]->str();
        int length = tmp.size();
        Json::Reader jsonReader;
        jsonReader.parse(tmp.c_str(), tmp.c_str() + length, *(jsonVal.get()));
        m_JsonRoots[*(tileIDs.at(i).get())] = jsonVal;
        delete out[i];
    }*/
    for(auto i = 0; i < urls.size(); i++) {
        delete out[i];
    }
    tileIDs.clear();
    urls.clear();
    return true;
}

//Returns jsonValue for a requested tileID
std::shared_ptr<Json::Value>
        MapzenVectorTileJson::GetData(std::string _tileID) {
    return m_JsonRoots[_tileID];
}
