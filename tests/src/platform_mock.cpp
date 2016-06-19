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

std::string setResourceRoot(const char* _path, std::string& _sceneResourceRoot) {

    std::string dir(_path);

    _sceneResourceRoot = std::string(dirname(&dir[0])) + '/';

    std::string base(_path);

    return std::string(basename(&base[0]));

}

std::string resolvePath(const char* _path, PathType _type, const char* _resourceRoot) {

    switch (_type) {
    case PathType::absolute:
    case PathType::internal:
        return std::string(_path);
    case PathType::resource:
        if (_resourceRoot) {
            return std::string(_resourceRoot) + _path;
        }
        return _path;
    }
    return "";
}

std::string stringFromFile(const char* _path, PathType _type, const char* _resourceRoot) {

    unsigned int length = 0;
    unsigned char* bytes = bytesFromFile(_path, _type, &length);

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;
}

unsigned char* bytesFromFile(const char* _path, PathType _type, unsigned int* _size, const char* _resourceRoot) {

    std::string path = resolvePath(_path, _type, _resourceRoot);

    std::ifstream resource(path.c_str(), std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", path.c_str());
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

std::string systemFontFallbackPath(int _importance, int _weightHint) {
    return "";
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
