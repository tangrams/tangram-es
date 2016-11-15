#include "platform.h"
#include "gl.h"
#include "context.h"
#include "urlWorker.h"

#include <libgen.h>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include <regex>

#define NUM_WORKERS 3

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

static bool s_isContinuousRendering = false;

static UrlWorker s_Workers[NUM_WORKERS];
static std::list<std::unique_ptr<UrlTask>> s_urlTaskQueue;

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void processNetworkQueue() {
    // attach workers to NetWorkerData
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

void requestRender() {
    setRenderRequest(true);
}

void setContinuousRendering(bool _isContinuous) {
    s_isContinuousRendering = _isContinuous;
}

bool isContinuousRendering() {
    return s_isContinuousRendering;
}

std::string stringFromFile(const char* _path) {

    size_t length = 0;
    unsigned char* bytes = bytesFromFile(_path, length);

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        _size = 0;
        return nullptr;
    }

    _size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * _size);

    resource.read(cdata, _size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

FontSourceHandle getFontHandle(const char* _path) {
    FontSourceHandle fontSourceHandle = [_path]() -> std::vector<char> {
        LOG("Loading font %s", _path);
        size_t dataSize = 0;

        auto cdata = bytesFromFile(_path, dataSize);
        auto data = std::vector<char>(cdata, cdata + dataSize);

        return std::move(data);
    };

    return fontSourceHandle;
}

std::vector<FontSourceHandle> systemFontFallbacksHandle() {
    std::vector<FontSourceHandle> handles;

    handles.push_back(getFontHandle(DEFAULT));
    handles.push_back(getFontHandle(FONT_AR));
    handles.push_back(getFontHandle(FONT_HE));
    handles.push_back(getFontHandle(FONT_JA));
    handles.push_back(getFontHandle(FALLBACK));

    return handles;
}

unsigned char* systemFont(const std::string& _name, const std::string& _weight, const std::string& _face, size_t* _size) {
    return nullptr;
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

    // Only clear this request if a worker has not started operating on it!
    // otherwise it gets too convoluted with curl!
    auto itr = s_urlTaskQueue.begin();
    while(itr != s_urlTaskQueue.end()) {
        if((*itr)->url == _url) {
            itr = s_urlTaskQueue.erase(itr);
        } else {
            itr++;
        }
    }
}

void setCurrentThreadPriority(int priority) {}

void initGLExtensions() {}
