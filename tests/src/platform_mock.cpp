#include "platform.h"

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>

#include <libgen.h>
//#include <sys/resource.h>

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

#include "log.h"

static bool s_isContinuousRendering = false;

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void requestRender() {
}

void setContinuousRendering(bool _isContinuous) {
    s_isContinuousRendering = _isContinuous;
}

bool isContinuousRendering() {
    return s_isContinuousRendering;
}

std::string stringFromFile(const char* _path) {

    std::string out;
    if (!_path || strlen(_path) == 0) { return out; }

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        return out;
    }

    resource.seekg(0, std::ios::end);
    out.resize(resource.tellg());
    resource.seekg(0, std::ios::beg);
    resource.read(&out[0], out.size());
    resource.close();

    return out;
}

std::vector<char> bytesFromFile(const char* _path) {
    if (!_path || strlen(_path) == 0) { return {}; }

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        LOG("Failed to read file at path: %s", _path);
        return {};
    }

    std::vector<char> data;
    size_t size = resource.tellg();
    data.resize(size);

    resource.seekg(std::ifstream::beg);

    resource.read(data.data(), size);
    resource.close();

    return data;
}

std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) {
    return {};
}

std::vector<FontSourceHandle> systemFontFallbacksHandle() {
    std::vector<FontSourceHandle> handles;

    handles.emplace_back(DEFAULT);
    handles.emplace_back(FONT_AR);
    handles.emplace_back(FONT_HE);
    handles.emplace_back(FONT_JA);
    handles.emplace_back(FALLBACK);

    return handles;
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {
    return true;
}

void cancelUrlRequest(const std::string& _url) {
}

void setCurrentThreadPriority(int priority){
}

void initGLExtensions() {
}
