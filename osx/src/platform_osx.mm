#ifdef PLATFORM_OSX

#import <Foundation/Foundation.h>
#import <utility>
#import <cstdio>
#import <cstdarg>
#import <fstream>
#import <regex>

#include <mutex>
#include <sys/resource.h>
#include <sys/syscall.h>

#include "platform_osx.h"
#include "gl/hardware.h"
#include "log.h"

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

static bool s_isContinuousRendering = false;

static bool s_stopUrlRequests = false;
static std::mutex s_urlRequestsMutex;

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

NSString* resolvePath(const char* _path) {

    NSString* pathString = [NSString stringWithUTF8String:_path];

    NSURL* resourceFolderUrl = [[NSBundle mainBundle] resourceURL];

    NSURL* resolvedUrl = [NSURL URLWithString:pathString
                                relativeToURL:resourceFolderUrl];

    return [resolvedUrl path];
}

std::string stringFromFile(const char* _path) {

    NSString* path = resolvePath(_path);
    NSString* str = [NSString stringWithContentsOfFile:path
                                          usedEncoding:NULL
                                                 error:NULL];

    if (str == nil) {
        LOGW("Failed to read file at path: %s", [path UTF8String]);
        return std::string();
    }

    return std::string([str UTF8String]);
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    NSString* path = resolvePath(_path);
    NSMutableData* data = [NSMutableData dataWithContentsOfFile:path];

    if (data == nil) {
        LOGW("Failed to read file at path: %s", [path UTF8String]);
        _size = 0;
        return nullptr;
    }

    _size = data.length;
    unsigned char* ptr = (unsigned char*)malloc(_size);
    [data getBytes:ptr length:_size];

    return ptr;
}

FontSourceHandle getFontHandle(const char* _path) {
    FontSourceHandle fontSourceHandle = [_path](size_t* _size) -> unsigned char* {
        LOG("Loading font %s", _path);

        auto cdata = bytesFromFile(_path, *_size);

        return cdata;
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

        {
            std::lock_guard<std::mutex> lock(s_urlRequestsMutex);

            if (s_stopUrlRequests) {
                LOGE("Response after Tangram shutdown.");
                return;
            }
        }

        NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;

        int statusCode = [httpResponse statusCode];

        std::vector<char> rawDataVec;

        if (error != nil) {

            LOGE("Response \"%s\" with error \"%s\".", response, [error.localizedDescription UTF8String]);

        } else if (statusCode < 200 || statusCode >= 300) {

            LOGE("Unsuccessful status code %d: \"%s\" from: %s",
                statusCode,
                [[NSHTTPURLResponse localizedStringForStatusCode: statusCode] UTF8String],
                [response.URL.absoluteString UTF8String]);
            _callback(std::move(rawDataVec));

        } else {

            int dataLength = [data length];
            rawDataVec.resize(dataLength);
            memcpy(rawDataVec.data(), (char *)[data bytes], dataLength);
            _callback(std::move(rawDataVec));

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

void finishUrlRequests() {

    {
        std::lock_guard<std::mutex> lock(s_urlRequestsMutex);
        s_stopUrlRequests = true;
    }

    [defaultSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for(NSURLSessionTask* task in dataTasks) {
            [task cancel];
        }
    }];
}

void setCurrentThreadPriority(int priority) {
    int tid = syscall(SYS_gettid);
    setpriority(PRIO_PROCESS, tid, priority);
}

void initGLExtensions() {
    Tangram::Hardware::supportsMapBuffer = true;
}

#endif //PLATFORM_OSX
