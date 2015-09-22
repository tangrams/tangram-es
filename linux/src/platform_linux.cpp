#ifdef PLATFORM_LINUX

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include "urlWorker.h"
#include "platform.h"
#include "gl.h"

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#define NUM_WORKERS 3

static bool s_isContinuousRendering = false;
static std::string s_resourceRoot;

static UrlWorker s_Workers[NUM_WORKERS];
static std::list<std::unique_ptr<UrlTask>> s_urlTaskQueue;

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void processNetworkQueue() {

    // check if any of the workers is done
    {
        for(auto& worker : s_Workers) {
            if(worker.isFinished() && !worker.isAvailable()) {
                auto result = worker.getResult();
                worker.reset();
                if(result->content.size() != 0) {
                    result->callback(std::move(result->content));
                }
            }
        }
    }

    // attach workers to NetWorkerData
    {
        auto taskItr = s_urlTaskQueue.begin();
        for(auto& worker : s_Workers) {
            if(taskItr == s_urlTaskQueue.end()) {
                break;
            }
            if(worker.isAvailable()) {
                worker.perform(std::move(*taskItr));
                taskItr = s_urlTaskQueue.erase(taskItr);
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

std::string setResourceRoot(const char* _path) {

    std::string dir(_path);

    s_resourceRoot = std::string(dirname(&dir[0])) + '/';

    std::string base(_path);

    return std::string(basename(&base[0]));

}

std::string stringFromResource(const char* _path) {

    unsigned int length = 0;
    unsigned char* bytes = bytesFromResource(_path, &length);

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;

}

unsigned char* bytesFromResource(const char* _path, unsigned int* _size) {

    std::string path = s_resourceRoot + _path;
    return bytesFromFileSystem(path.c_str(), _size);

}

unsigned char* bytesFromFileSystem(const char* _path, unsigned int* _size) {

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

// No system fonts implementation (yet!)
std::string systemFontPath(const std::string& _name, const std::string& _weight, const std::string& _face) {
    return "";
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {

    std::unique_ptr<UrlTask> task(new UrlTask(_url, _callback));
    for(auto& worker : s_Workers) {
        if(worker.isAvailable()) {
            worker.perform(std::move(task));
            return true;
        }
    }
    s_urlTaskQueue.push_back(std::move(task));
    return true;

}

void cancelUrlRequest(const std::string& _url) {

    // Only clear this request if a worker has not started operating on it!! otherwise it gets too convoluted with curl!
    auto itr = s_urlTaskQueue.begin();
    while(itr != s_urlTaskQueue.end()) {
        if((*itr)->url == _url) {
            itr = s_urlTaskQueue.erase(itr);
        } else {
            itr++;
        }
    }
}

void setCurrentThreadPriority(int priority){
    int tid = syscall(SYS_gettid);
    //int  p1 = getpriority(PRIO_PROCESS, tid);

    setpriority(PRIO_PROCESS, tid, priority);

    //int  p2 = getpriority(PRIO_PROCESS, tid);
    //logMsg("set niceness: %d -> %d\n", p1, p2);
}

#endif
