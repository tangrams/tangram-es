#include "platform.h"

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>

#include <libgen.h>
//#include <sys/resource.h>

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


unsigned char* systemFont(const std::string& _name, const std::string& _weight, const std::string& _face, size_t* _size) {
    return nullptr;
}

std::vector<FontSourceHandle> systemFontFallbacksHandle() {
    return {};
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
