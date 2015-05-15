#ifdef PLATFORM_LINUX

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include "platform.h"
#include "gl.h"

static bool s_isContinuousRendering = false;
static std::function<void(std::vector<char>&&, TileID, int)> networkCallback;

static NetworkWorker s_Workers[NUM_WORKERS];
static std::list<std::unique_ptr<NetWorkerData>> s_WorkerDataQueue;

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void processNetworkQueue() {

    // attach workers to NetWorkerData
    {
        auto workerDataItr = s_WorkerDataQueue.begin();
        for(auto& worker : s_Workers) {
            if(workerDataItr == s_WorkerDataQueue.end()) {
                break;
            }
            if(worker.isAvailable()) {
                worker.perform(std::move(*workerDataItr));
                workerDataItr = s_WorkerDataQueue.erase(workerDataItr);
            }
        }
    }

    // check if any of the workers is done
    {
        for(auto& worker : s_Workers) {
            if(worker.isFinished() && !worker.isAvailable()) {
                auto resultData = worker.getWorkerResult();
                worker.reset();
                if(resultData->rawData.size() != 0) {
                    networkCallback(std::move(resultData->rawData), resultData->tileID, resultData->dataSourceID);
                } else {
                    logMsg("Something went wrong during network fetch of tile: [%d, %d, %d]\n", resultData->tileID.x, resultData->tileID.y, resultData->tileID.z);
                }
            }
        }
    }
}

void requestRender() {
    
    glfwPostEmptyEvent();
    
}

void setContinuousRendering(bool _isContinuous) {
    
    s_isContinuousRendering = _isContinuous;
    
}

bool isContinuousRendering() {
    
    return s_isContinuousRendering;
    
}

std::string stringFromResource(const char* _path) {
    std::string into;

    std::ifstream file;
    std::string buffer;

    file.open(_path);
    if(!file.is_open()) {
        logMsg("Failed to open file at path: %s\n", _path);
        return std::string();
    }

    while(!file.eof()) {
        getline(file, buffer);
        into += buffer + "\n";
    }

    file.close();
    return into;
}

unsigned char* bytesFromResource(const char* _path, unsigned int* _size) {
    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        *_size = 0;
        return nullptr;
    }

    *_size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * (*_size));

    resource.read(cdata, *_size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

bool startNetworkRequest(const std::string& _url, const TileID& _tileID, const int _dataSourceID) {

    std::unique_ptr<NetWorkerData> workerData(new NetWorkerData(_url, _tileID, _dataSourceID));
    for(auto& worker : s_Workers) {
        if(worker.isAvailable()) {
            worker.perform( std::move(workerData) ); 
            return true;
        }
    }
    s_WorkerDataQueue.push_back( std::move(workerData) );
    return true;

}

void cancelNetworkRequest(const std::string& _url) {

    // Only clear this request if a worker has not started operating on it!! otherwise it gets too convoluted with curl!
    auto itr = s_WorkerDataQueue.begin();
    while(itr != s_WorkerDataQueue.end()) {
        if((*itr)->url == _url) {
            itr = s_WorkerDataQueue.erase(itr);
        } else {
            itr++;
        }
    }
}

void setNetworkRequestCallback(std::function<void(std::vector<char>&&, TileID, int)>&& _callback) {
    networkCallback = _callback;
}


#endif
