#ifdef PLATFORM_IOS

#import <Foundation/Foundation.h>
#import <utility>
#import <cstdio>
#import <cstdarg>
#import <fstream>

#include "platform.h"
#include "ViewController.h"

static ViewController* viewController;
static std::function<void(std::vector<char>&&, TileID, int)> networkCallback;

void setViewController(ViewController* _controller) {
    
    viewController = _controller;
    
}

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

}

void requestRender() {
    
    [viewController renderOnce];
    
}

void setContinuousRendering(bool _isContinuous) {
    
    [viewController setContinuous:_isContinuous];
    
}

bool isContinuousRendering() {
    
    return [viewController continuous];
    
}

NSString* resolveResourcePath(const char* _path) {
    
    NSString* resourcePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/"];
    NSString* internalPath = [NSString stringWithUTF8String:_path];
    return [resourcePath stringByAppendingString:internalPath];
    
}

std::string stringFromResource(const char* _path) {
    
    NSString* path = resolveResourcePath(_path);
    NSString* str = [NSString stringWithContentsOfFile:path
                                              encoding:NSASCIIStringEncoding
                                                 error:NULL];

    if (str == nil) {
        logMsg("Failed to read file at path: %s\n", _path);
        return std::move(std::string());
    }
    
    return std::move(std::string([str UTF8String]));
}

unsigned char* bytesFromResource(const char* _path, unsigned int* _size) {

    NSString* path = resolveResourcePath(_path);
    std::ifstream resource([path UTF8String], std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        *_size = 0;
        return nullptr;
    }

    *_size = (unsigned int)resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * (*_size));

    resource.read(cdata, *_size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

bool startNetworkRequest(const std::string& _url, const TileID& _tileID, const int _dataSourceID) {

    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];
    
    if(! [viewController networkRequestWithUrl:nsUrl
                         TileID:_tileID
                         DataSourceID:[NSNumber numberWithInt:_dataSourceID] ] ) {
        
        logMsg("\"networkRequest\" returned false");
        return false;
        
    }
    
    return true;

}

void cancelNetworkRequest(const std::string& _url) {
    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];
    [viewController cancelNetworkRequestWithUrl:nsUrl];
}

void setNetworkRequestCallback(std::function<void(std::vector<char>&&, TileID, int)>&& _callback) {
    networkCallback = _callback;
}

void networkDataBridge(std::vector<char>& _rawData, TileID _tileID, int _dataSource) {
    networkCallback(std::move(_rawData), _tileID, _dataSource);
}

#endif //PLATFORM_IOS
