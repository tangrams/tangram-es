#include "osxPlatform.h"
#include "gl/hardware.h"
#include "log.h"
#include <map>

#import "TGFontConverter.h"
#import <cstdarg>
#import <cstdio>
#import <AppKit/AppKit.h>

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

namespace Tangram {

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void setCurrentThreadPriority(int priority) {
    // POSIX thread priority is between -20 (highest) and 19 (lowest),
    // NSThread priority is between 0.0 (lowest) and 1.0 (highest).
    double p = (20 - priority) / 40.0;
    [[NSThread currentThread] setThreadPriority:p];
}

void initGLExtensions() {
    Tangram::Hardware::supportsMapBuffer = true;
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

void OSXPlatform::requestRender() const {
    glfwPostEmptyEvent();
}

OSXPlatform::OSXPlatform() : m_stopUrlRequests(false) {
    NSURLSessionConfiguration *defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];

    m_defaultSession = [NSURLSession sessionWithConfiguration: defaultConfigObject];

    NSString *cachePath = [NSTemporaryDirectory() stringByAppendingPathComponent:@"/tile_cache"];
    NSURLCache *tileCache = [[NSURLCache alloc] initWithMemoryCapacity: 4 * 1024 * 1024 diskCapacity: 30 * 1024 * 1024 diskPath: cachePath];
    defaultConfigObject.URLCache = tileCache;
    defaultConfigObject.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
    defaultConfigObject.timeoutIntervalForRequest = 30;
    defaultConfigObject.timeoutIntervalForResource = 60;
}

OSXPlatform::~OSXPlatform() {
    {
        std::lock_guard<std::mutex> guard(m_urlRequestMutex);
        m_stopUrlRequests = true;
    }

    [m_defaultSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for(NSURLSessionTask* task in dataTasks) {
            [task cancel];
        }
    }];
}

std::string OSXPlatform::stringFromFile(const char* _path) const {
    NSString* path = resolvePath(_path);
    std::string data;

    if (!path) { return data; }

    data = Platform::stringFromFile([path UTF8String]);
    return data;
}

std::vector<char> loadNSFont(NSFont* _font) {
    if (!_font) {
        return {};
    }

    CGFontRef fontRef = CGFontCreateWithFontName((CFStringRef)_font.fontName);

    if (!fontRef) {
        return {};
    }

    std::vector<char> data = [TGFontConverter fontDataForCGFont:fontRef];

    CGFontRelease(fontRef);

    if (data.empty()) {
        LOG("CoreGraphics font failed to decode");
    }

    return data;
}

bool allowedFamily(NSString* familyName) {
    const NSArray<NSString *> *allowedFamilyList = @[ @"Hebrew", @"Kohinoor", @"Gumurki", @"Thonburi", @"Tamil",
                                                    @"Gurmukhi", @"Kailasa", @"Sangam", @"PingFang", @"Geeza",
                                                    @"Mishafi", @"Farah", @"Hiragino", @"Gothic" ];

    for (NSString* allowedFamily in allowedFamilyList) {
        if ( [familyName containsString:allowedFamily] ) { return true; }
    }
    return false;
}

std::vector<FontSourceHandle> OSXPlatform::systemFontFallbacksHandle() const {
    std::vector<FontSourceHandle> handles;

    NSFontManager *manager = [NSFontManager sharedFontManager];
    NSArray<NSString *> *fallbacks = [manager availableFontFamilies];

    for (NSString* fallback in fallbacks) {
        if (!allowedFamily(fallback)) { continue; }

        for (NSArray* familyFont in [manager availableMembersOfFontFamily:fallback]) {
            NSString* fontName = familyFont[0];
            NSString* fontStyle = familyFont[1];
            if ( ![fontName containsString:@"-"] || [fontStyle isEqualToString:@"Regular"]) {
                handles.emplace_back([fontName]() {
                    auto data = loadNSFont([NSFont fontWithName:fontName size:1.0]);
                    return data;
                });
                break;
            }
        }
    }

    return handles;
}

FontSourceHandle OSXPlatform::systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const {
    static std::map<int, CGFloat> weightTraits = {
        {100, NSFontWeightUltraLight},
        {100, NSFontWeightUltraLight},
        {200, NSFontWeightThin},
        {300, NSFontWeightLight},
        {400, NSFontWeightRegular},
        {500, NSFontWeightMedium},
        {600, NSFontWeightSemibold},
        {700, NSFontWeightBold},
        {800, NSFontWeightHeavy},
        {900, NSFontWeightBlack},
    };

    static std::map<std::string, NSFontSymbolicTraits> fontTraits = {
        {"italic", NSFontItalicTrait},
        {"bold", NSFontBoldTrait},
        {"expanded", NSFontExpandedTrait},
        {"condensed", NSFontCondensedTrait},
        {"monospace", NSFontMonoSpaceTrait},
    };

    NSFont* font = [NSFont fontWithName:[NSString stringWithUTF8String:_name.c_str()] size:1.0];

    if (font == nil) {
        // Get the default system font
        if (_weight.empty()) {
            font = [NSFont systemFontSize:1.0];
        } else {
            int weight = atoi(_weight.c_str());

            // Default to 400 boldness
            weight = (weight == 0) ? 400 : weight;

            // Map weight value to range [100..900]
            weight = std::min(std::max(100, (int)floor(weight / 100.0 + 0.5) * 100), 900);

            font = [NSFont systemFontOfSize:1.0 weight:weightTraits[weight]];
        }
    }

    if (_face != "normal") {
        NSFontSymbolicTraits traits;
        NSFontDescriptor* descriptor = [font fontDescriptor];

        auto it = fontTraits.find(_face);
        if (it != fontTraits.end()) {
            traits = it->second;

            // Create a new descriptor with the symbolic traits
            descriptor = [descriptor fontDescriptorWithSymbolicTraits:traits];

            if (descriptor != nil) {
                font = [NSFont fontWithDescriptor:descriptor size:1.0];
            }
        }
    }

    return FontSourceHandle([font]() { return loadNSFont(font); });
}

bool OSXPlatform::startUrlRequest(const std::string& _url, UrlCallback _callback) {

    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];

    void (^handler)(NSData*, NSURLResponse*, NSError*) = ^void (NSData* data, NSURLResponse* response, NSError* error) {
        {
            std::lock_guard<std::mutex> guard(m_urlRequestMutex);

            if (m_stopUrlRequests) {
                LOGE("Response after Tangram shutdown.");
                return;
            }
        }

        NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;

        int statusCode = [httpResponse statusCode];

        std::vector<char> rawDataVec;

        if (error != nil) {

            if ([error.domain isEqualToString:NSURLErrorDomain] && error.code == NSURLErrorCancelled) {
                LOGD("Request cancelled: %s", [response.URL.absoluteString UTF8String]);
            } else {
                LOGE("Response \"%s\" with error \"%s\".", response, [error.localizedDescription UTF8String]);
            }

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

    NSURLSessionDataTask* dataTask = [m_defaultSession dataTaskWithURL:[NSURL URLWithString:nsUrl] completionHandler:handler];

    [dataTask resume];

    return true;

}

void OSXPlatform::cancelUrlRequest(const std::string& _url) {

    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];

    [m_defaultSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for(NSURLSessionTask* task in dataTasks) {
            if([[task originalRequest].URL.absoluteString isEqualToString:nsUrl]) {
                [task cancel];
                break;
            }
        }
    }];
}

} // namespace Tangram
