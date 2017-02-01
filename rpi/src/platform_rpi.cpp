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

#include "log.h"

#define NUM_WORKERS 3

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void RPiPlatform::processNetworkQueue() {
    // attach workers to NetWorkerData
    auto taskItr = m_urlTaskQueue.begin();
    for(auto& worker : m_workers) {
        if(taskItr == m_urlTaskQueue.end()) {
            break;
        }
        if(worker.isAvailable()) {
            worker.perform(std::move(*taskItr), static_cast<const Platform&>(*this));
            taskItr = m_urlTaskQueue.erase(taskItr);
        }
    }
}

void RPiPlatform::requestRender() const {
    glfwPostEmptyEvent();
}

std::vector<FontSourceHandle> RPiPlatform::systemFontFallbacksHandle() const {
    std::vector<FontSourceHandle> handles;

    handles.emplace_back(DEFAULT);
    handles.emplace_back(FONT_AR);
    handles.emplace_back(FONT_HE);
    handles.emplace_back(FONT_JA);
    handles.emplace_back(FALLBACK);

    return handles;
}

bool RPiPlatform::startUrlRequest(const std::string& _url, UrlCallback _callback) {

    std::unique_ptr<UrlTask> task(new UrlTask(_url, _callback));
    for (auto& worker : m_workers) {
        if (worker.isAvailable()) {
            worker.perform(std::move(task), static_cast<const Platform&>(*this));
            return true;
        }
    }
    m_urlTaskQueue.push_back(std::move(task));
    return true;

}

void RPiPlatform::cancelUrlRequest(const std::string& _url) {

    // Only clear this request if a worker has not started operating on it!!
    // otherwise it gets too convoluted with curl!
    auto itr = m_urlTaskQueue.begin();
    while (itr != m_urlTaskQueue.end()) {
        if ((*itr)->url == _url) {
            itr = m_urlTaskQueue.erase(itr);
        } else {
            itr++;
        }
    }
}

RPiPlatform::~RPiPlatform() {
    for(auto& worker : m_workers) {
        worker.join();
    }
    m_urlTaskQueue.clear();
}

void setCurrentThreadPriority(int priority) {}

void initGLExtensions() {}
