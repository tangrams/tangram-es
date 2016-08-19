#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>

#include "platform.h"
#include "gl.h"

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

static std::string s_resourceRoot;

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void requestRender() {
}

void setContinuousRendering(bool _isContinuous) {
}

bool isContinuousRendering() {
    return true;
}

std::string stringFromFile(const char* _path) {

    size_t length = 0;
    unsigned char* bytes = bytesFromFile(_path, length);
    if (!bytes) { return {}; }

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

    char* cdata = (char*) malloc(sizeof(char) * (_size));

    resource.read(cdata, _size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

// No system fonts implementation (yet!)
std::string systemFontPath(const std::string& _name, const std::string& _weight, const std::string& _face) {
    return "";
}

void setCurrentThreadPriority(int priority){
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {
    return true;

}

void cancelUrlRequest(const std::string& _url) {
}

void initGLExtensions() {}
