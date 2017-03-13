#ifdef PLATFORM_IOS

#import <UIKit/UIKit.h>
#import <utility>
#import <cstdio>
#import <cstdarg>
#import <fstream>
#import <regex>
#import <iostream>

#import "TGMapViewController.h"
#import "TGFontConverter.h"
#import "TGHttpHandler.h"
#import "platform_ios.h"
#import "log.h"

namespace Tangram {

NSString* resolvePath(const char* _path) {
    NSString* pathString = [NSString stringWithUTF8String:_path];

    NSURL* resourceFolderUrl = [[NSBundle mainBundle] resourceURL];

    NSURL* resolvedUrl = [NSURL URLWithString:pathString
                                relativeToURL:resourceFolderUrl];

    NSString* pathInAppBundle = [resolvedUrl path];

    NSFileManager* fileManager = [NSFileManager defaultManager];

    if ([fileManager fileExistsAtPath:pathInAppBundle]) {
        return pathInAppBundle;
    }

    LOGW("Failed to resolve path: %s", _path);

    return nil;
}

std::vector<char> loadUIFont(UIFont* _font) {
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

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void setCurrentThreadPriority(int priority) {
    // No-op
}

void initGLExtensions() {
    // No-op
}

iOSPlatform::iOSPlatform(TGMapViewController* _viewController) :
    Platform(),
    m_viewController(_viewController) {}

void iOSPlatform::requestRender() const {
    [m_viewController renderOnce];
}

void iOSPlatform::setContinuousRendering(bool _isContinuous) {
    Platform::setContinuousRendering(_isContinuous);
    [m_viewController setContinuous:_isContinuous];
}

std::string iOSPlatform::resolveAssetPath(const std::string& _path) const {
    NSString* path = resolvePath(_path.c_str());
    return [path UTF8String];
}

std::vector<char> iOSPlatform::bytesFromFile(const char* _path) const {
    NSString* path = resolvePath(_path);

    if (!path) { return {}; }

    auto data = Platform::bytesFromFile([path UTF8String]);
    return data;
}

std::string iOSPlatform::stringFromFile(const char* _path) const {
    NSString* path = resolvePath(_path);
    std::string data;

    if (!path) { return data; }

    data = Platform::stringFromFile([path UTF8String]);
    return data;
}

std::vector<FontSourceHandle> iOSPlatform::systemFontFallbacksHandle() const {
    NSArray* fallbacks = [UIFont familyNames];

    std::vector<FontSourceHandle> handles;

    for (id fallback in fallbacks) {

        handles.emplace_back([fallback]() {
            auto data = loadUIFont([UIFont fontWithName:fallback size:1.0]);
            return data;
        });
    }

    return handles;
}

std::vector<char> iOSPlatform::systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const {
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

    UIFont* font = [UIFont fontWithName:[NSString stringWithUTF8String:_name.c_str()] size:1.0];

    if (font == nil) {
        // Get the default system font
        if (_weight.empty()) {
            font = [UIFont systemFontOfSize:1.0];
        } else {
            int weight = std::atoi(_weight.c_str());

            // Default to 400 boldness
            weight = (weight == 0) ? 400 : weight;

            // Map weight value to range [100..900]
            weight = std::min(std::max(100, (int)floor(weight / 100.0 + 0.5) * 100), 900);

            font = [UIFont systemFontOfSize:1.0 weight:weightTraits[weight]];
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
                font = [UIFont fontWithDescriptor:descriptor size:1.0];
            }
        }
    }

    return loadUIFont(font);
}

bool iOSPlatform::startUrlRequest(const std::string& _url, UrlCallback _callback) {
    TGHttpHandler* httpHandler = [m_viewController httpHandler];

    if (!httpHandler) {
        return false;
    }

    TGDownloadCompletionHandler handler = ^void (NSData* data, NSURLResponse* response, NSError* error) {

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

    NSString* url = [NSString stringWithUTF8String:_url.c_str()];
    [httpHandler downloadRequestAsync:url completionHandler:handler];

    return true;
}

void iOSPlatform::cancelUrlRequest(const std::string& _url) {
    TGHttpHandler* httpHandler = [m_viewController httpHandler];

    if (!httpHandler) { return; }

    NSString* url = [NSString stringWithUTF8String:_url.c_str()];
    [httpHandler cancelDownloadRequestAsync:url];
}

} // namespace Tangram

#endif //PLATFORM_IOS
