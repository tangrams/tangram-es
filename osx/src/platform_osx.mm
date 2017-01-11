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

    NSFileManager* fileManager = [NSFileManager defaultManager];

    NSString* pathInAppBundle = [resolvedUrl path];

    if ([fileManager fileExistsAtPath:pathInAppBundle]) {
        return pathInAppBundle;
    }

    LOGW("Failed to resolve path: %s", _path);

    return nil;
}

bool bytesFromFileSystem(const char* _path, std::function<char*(size_t)> _allocator) {
    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        return false;
    }

    size_t size = resource.tellg();
    char* cdata = _allocator(size);

    resource.seekg(std::ifstream::beg);
    resource.read(cdata, size);
    resource.close();

    return true;
}

std::string stringFromFile(const char* _path) {
    NSString* path = resolvePath(_path);

    if (!path) {
        return "";
    }

    std::string data;

    auto allocator = [&](size_t size) {
        data.resize(size);
        return &data[0];
    };

    bytesFromFileSystem([path UTF8String], allocator);

    return data;
}

std::vector<char> bytesFromFile(const char* _path) {
    NSString* path = resolvePath(_path);

    if (!path) {
        return {};
    }

    std::vector<char> data;

    auto allocator = [&](size_t size) {
        data.resize(size);
        return data.data();
    };

    bytesFromFileSystem([path UTF8String], allocator);

    return data;
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

std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) {
    return {};
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
