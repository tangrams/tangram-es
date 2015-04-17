#ifdef PLATFORM_LINUX

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>

#include <curl/curl.h>

#include "platform.h"
#include "gl.h"

static bool s_isContinuousRendering = false;

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
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

//write_data call back from CURLOPT_WRITEFUNCTION
//responsible to read and fill "stream" with the data.
static size_t write_data(void *_ptr, size_t _size, size_t _nmemb, void *_stream) {
    ((std::stringstream*) _stream)->write(reinterpret_cast<char *>(_ptr), _size * _nmemb);
    return _size * _nmemb;
}

bool streamFromHttpASync(const std::string& _url, const TileID& _tileID, const int _dataSourceID) {

    std::stringstream _rawData;
    CURL* curlHandle = curl_easy_init();

    // set up curl to perform fetch
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &_rawData);
    curl_easy_setopt(curlHandle, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "gzip");
    
    logMsg("Fetching URL with curl: %s\n", _url.c_str());

    CURLcode result = curl_easy_perform(curlHandle);
    
    curl_easy_cleanup(curlHandle);
    if (result != CURLE_OK) {
        logMsg("curl_easy_perform failed: %s\n", curl_easy_strerror(result));
        return false;
    } else {
        return true;
    }
}

void cancelNetworkRequest(const std::string& _url) {
    //TODO
}

void setNetworkRequestCallback(std::function<void(std::vector<char>&&, TileID, int)>&& _callback) {
    //TODO
}


#endif
