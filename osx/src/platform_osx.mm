#ifdef PLATFORM_OSX

#import <Foundation/Foundation.h>
#import <utility>
#import <cstdio>
#import <cstdarg>
#import <fstream>
#include <sys/resource.h>
#include <sys/syscall.h>

#include "platform.h"
#include "gl.h"

static bool s_isContinuousRendering = false;

NSURLSession* defaultSession;

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

    *_size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * (*_size));

    resource.read(cdata, *_size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

// Does not provide implementation for this (yet!)
unsigned char* bytesFromExtMemory(const char* _path, unsigned int* _size) {
    return nullptr;
}

// No system fonts implementation (yet!)
std::string deviceFontsPath() {
    return "";
}

// TODO: Costruct font filename for osx
std::string constructFontFilename(const std::string& _name, const std::string& _weight, const std::string& _face) {
    return "";
}

bool parseTypeFaceFontsInfo(const std::string& _typefaceInput, std::string& _fontName, float& _fontSize) {

    char str[4][40];
    float size;
    int num = sscanf(_typefaceInput.c_str(), "%s %s %s %s", str[0], str[1], str[2], str[3]);
    switch(num) {
        case 1:
            _fontName = str[0];
            break;
        case 2:
            try {
                size = std::stof(std::string(str[0]));
                _fontSize = size;
                _fontName = str[1];
            } catch (const std::invalid_argument& e) {
                _fontName = std::string(str[1]) + "-" + std::string(str[0]);
            }
            break;
        case 3:
            try {
                size = std::stof(std::string(str[1]));
                _fontSize = size;
                _fontName = std::string(str[2]) + "-" + std::string(str[0]);
            } catch (const std::invalid_argument& e) {
                _fontName = std::string(str[2]) + "-" + std::string(str[1]) + std::string(str[0]);
            }
            break;
        case 4:
            try {
                size = std::stof(std::string(str[2]));
                _fontSize = size;
                _fontName = std::string(str[3]) + "-" + std::string(str[1]) + std::string(str[0]);
            } catch (const std::invalid_argument& e) {
                _fontName = std::string(str[3]) + "-" + std::string(str[2]) + std::string(str[1]) + std::string(str[0]);
            }
            break;
        case 0:
        default:
            return false;
    }
    return true;
}

void NSurlInit() {
    NSURLSessionConfiguration *defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSString *cachePath = [NSTemporaryDirectory() stringByAppendingPathComponent:@"/tile_cache"];
    NSURLCache *tileCache = [[NSURLCache alloc] initWithMemoryCapacity: 4 * 1024 * 1024 diskCapacity: 30 * 1024 * 1024 diskPath: cachePath];
    defaultConfigObject.URLCache = tileCache;
    defaultConfigObject.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
    defaultConfigObject.timeoutIntervalForRequest = 30;
    defaultConfigObject.timeoutIntervalForResource = 60;
    
    defaultSession = [NSURLSession sessionWithConfiguration: defaultConfigObject];
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {

    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];
    
    void (^handler)(NSData*, NSURLResponse*, NSError*) = ^void (NSData* data, NSURLResponse* response, NSError* error) {
        
        if(error == nil) {
            
            int dataLength = [data length];
            std::vector<char> rawDataVec;
            rawDataVec.resize(dataLength);
            memcpy(rawDataVec.data(), (char *)[data bytes], dataLength);
            _callback(std::move(rawDataVec));
            
        } else {
            
            logMsg("ERROR: response \"%s\" with error \"%s\".\n", response, std::string([error.localizedDescription UTF8String]).c_str());

        }
        
    };
    
    NSURLSessionDataTask* dataTask = [defaultSession dataTaskWithURL:[NSURL URLWithString:nsUrl] completionHandler:handler];
    
    [dataTask resume];
    
    return true;
    
}

void cancelUrlRequest(const std::string& _url) {
    
    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];
   
    [defaultSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for(NSURLSessionTask* task in dataTasks) {
            if([[task originalRequest].URL.absoluteString isEqualToString:nsUrl]) {
                [task cancel];
                break;
            }
        }
    }];
}

void setCurrentThreadPriority(int priority) {
    int tid = syscall(SYS_gettid);
    setpriority(PRIO_PROCESS, tid, priority);
}

#endif //PLATFORM_OSX
