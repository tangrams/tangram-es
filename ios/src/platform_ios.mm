#ifdef PLATFORM_IOS

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <utility>
#import <cstdio>
#import <cstdarg>
#import <fstream>
#import <regex>

#include <iostream>
#include <fstream>

#import "TGMapViewController.h"
#import "CGFontConverter.h"

#include "platform_ios.h"
#include "log.h"

static TGMapViewController* viewController;
static NSBundle* tangramFramework;
NSURLSession* defaultSession;

void init(TGMapViewController* _controller) {

    viewController = _controller;

    /* Setup NSURLSession configuration : cache path and size */
    NSURLSessionConfiguration *defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSString *cachePath = @"/tile_cache";
    NSURLCache *tileCache = [[NSURLCache alloc] initWithMemoryCapacity: 4 * 1024 * 1024 diskCapacity: 30 * 1024 * 1024 diskPath: cachePath];
    defaultConfigObject.URLCache = tileCache;
    defaultConfigObject.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
    defaultConfigObject.timeoutIntervalForRequest = 30;
    defaultConfigObject.timeoutIntervalForResource = 60;

    /* create a default NSURLSession using the defaultConfigObject*/
    defaultSession = [NSURLSession sessionWithConfiguration: defaultConfigObject ];

    // Get handle to tangram framework
    tangramFramework = [NSBundle bundleWithIdentifier:@"com.mapzen.tangramMap"];
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

NSString* resolvePath(const char* _path) {

    NSString* path = [NSString stringWithUTF8String:_path];

    if (*_path == '/') { return path; }

    NSString* resources = [[NSBundle mainBundle] resourcePath];
    NSString* fullBundlePath = [resources stringByAppendingPathComponent:path];
    NSFileManager* fileManager = [NSFileManager defaultManager];

    if ([fileManager fileExistsAtPath:fullBundlePath]) {
        return fullBundlePath;
    }

    if (tangramFramework) {
        NSString* resources = [tangramFramework resourcePath];
        fullBundlePath = [resources stringByAppendingPathComponent:path];

        if ([fileManager fileExistsAtPath:fullBundlePath]) {
            return fullBundlePath;
        }
    }

    return nil;
}

std::string stringFromFile(const char* _path) {

    NSString* path = resolvePath(_path);

    if (!path) {
        return "";
    }

    NSString* str = [NSString stringWithContentsOfFile:path
                                          usedEncoding:NULL
                                                 error:NULL];

    if (str == nil) {
        LOGW("Failed to read file at path: %s\n", [path UTF8String]);
        return std::string();
    }

    return std::string([str UTF8String]);
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    NSString* path = resolvePath(_path);
    NSMutableData* data = [NSMutableData dataWithContentsOfFile:path];

    if (data == nil) {
        LOGW("Failed to read file at path: %s\n", [path UTF8String]);
        _size = 0;
        return nullptr;
    }

    _size = data.length;
    unsigned char* ptr = (unsigned char*)malloc(_size);
    [data getBytes:ptr length:_size];

    return ptr;
}

unsigned char* loadUIFont(UIFont* _font, size_t* _size) {

    LOG("Loading system font %s", [_font.fontName UTF8String]);

    CGFontRef fontRef = CGFontCreateWithFontName((CFStringRef)_font.fontName);

    if (!fontRef) {
        *_size = 0;
        return nullptr;
    }

    NSData* data = [CGFontConverter fontDataForCGFont:fontRef];

    CGFontRelease(fontRef);

    if (data == nil) {
        LOG("CoreGraphics font failed to decode");

        *_size = 0;
        return nullptr;
    }

    unsigned char* bytes = (unsigned char*)malloc([data length]);
    std::memcpy(bytes, (unsigned char*)[data bytes], [data length]);

    *_size = [data length];

    return bytes;
}

std::vector<FontSourceHandle> systemFontFallbacksHandle() {
    UIFont* arabic = [UIFont fontWithName:@"Geeza Pro" size:0.0];

    FontSourceHandle fontSourceHandle = [arabic]() -> std::vector<char> {
        size_t dataSize = 0;

        auto cdata = loadUIFont(arabic, &dataSize);
        auto data = std::vector<char>(cdata, cdata + dataSize);

        // TODO: reduce copies, move vector back

        return data;
    };

    std::vector<FontSourceHandle> handles;

    handles.push_back(fontSourceHandle);

    return handles;
}

unsigned char* systemFont(const std::string& _name, const std::string& _weight, const std::string& _face, size_t* _size) {

    static std::map<int, CGFloat> weightTraits = {
        {100, UIFontWeightUltraLight},
        {200, UIFontWeightThin},
        {300, UIFontWeightLight},
        {400, UIFontWeightRegular},
        {500, UIFontWeightMedium},
        {600, UIFontWeightSemibold},
        {700, UIFontWeightBold},
        {800, UIFontWeightHeavy},
        {900, UIFontWeightBlack},
    };

    static std::map<std::string, UIFontDescriptorSymbolicTraits> fontTraits = {
        {"italic", UIFontDescriptorTraitItalic},
        {"oblique", UIFontDescriptorTraitItalic},
        {"bold", UIFontDescriptorTraitBold},
        {"expanded", UIFontDescriptorTraitExpanded},
        {"condensed", UIFontDescriptorTraitCondensed},
        {"monospace", UIFontDescriptorTraitMonoSpace},
    };

    UIFont* font = [UIFont fontWithName:[NSString stringWithUTF8String:_name.c_str()] size:0.0];

    if (font == nil) {
        // Get the default system font
        if (_weight.empty()) {
            font = [UIFont systemFontOfSize:0.0];
        } else {
            int weight = std::atoi(_weight.c_str());

            // Default to 400 boldness
            weight = (weight == 0) ? 400 : weight;

            // Map weight value to range [100..900]
            weight = std::min(std::max(100, (int)floor(weight / 100.0 + 0.5) * 100), 900);

            font = [UIFont systemFontOfSize:0.0 weight:weightTraits[weight]];
        }
    }

    if (_face != "normal") {
        UIFontDescriptorSymbolicTraits traits;
        UIFontDescriptor* descriptor = [font fontDescriptor];

        auto it = fontTraits.find(_face);
        if (it != fontTraits.end()) {
            traits = it->second;

            // Create a new descriptor with the symbolic traits
            descriptor = [descriptor fontDescriptorWithSymbolicTraits:traits];

            if (descriptor != nil) {
                font = [UIFont fontWithDescriptor:descriptor size:0.0];
            }
        }
    }

    return loadUIFont(font, _size);
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {

    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];

    void (^handler)(NSData*, NSURLResponse*, NSError*) = ^void (NSData* data, NSURLResponse* response, NSError* error) {

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

    NSURLSessionDataTask* dataTask = [defaultSession dataTaskWithURL:[NSURL URLWithString:nsUrl]
                                                    completionHandler:handler];

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

void setCurrentThreadPriority(int priority) {}

void initGLExtensions() {}

#endif //PLATFORM_IOS
