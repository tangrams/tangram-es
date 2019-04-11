#import "appleAllowedFonts.h"
#import "TGURLHandler.h"
#import "TGMapView+Internal.h"
#import <UIKit/UIKit.h>

#include "iosPlatform.h"
#include "log.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <map>

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
    // No-op
}

iOSPlatform::iOSPlatform(__weak TGMapView* _mapView) :
    Platform(),
    m_mapView(_mapView) {}

void iOSPlatform::requestRender() const {
    __strong TGMapView* mapView = m_mapView;

    if (!mapView) {
        return;
    }

    [mapView requestRender];
}

void iOSPlatform::setContinuousRendering(bool _isContinuous) {
    Platform::setContinuousRendering(_isContinuous);
    __strong TGMapView* mapView = m_mapView;

    if (!mapView) {
        return;
    }

    mapView.continuous = _isContinuous;
}

std::vector<FontSourceHandle> iOSPlatform::systemFontFallbacksHandle() const {

    NSArray<NSString *> *fallbacks = [UIFont familyNames];

    std::vector<FontSourceHandle> handles;
    handles.reserve([fallbacks count]);

    for (NSString* fallback in fallbacks) {

        if (!allowedFamily(fallback)) { continue; }

        for (NSString* fontName in [UIFont fontNamesForFamilyName:fallback]) {
            if ( ![fontName containsString:@"-"] || [fontName containsString:@"-Regular"]) {
                handles.emplace_back(std::string(fontName.UTF8String));
                break;
            }
        }
    }

    return handles;
}

FontSourceHandle iOSPlatform::systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const {
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

    if (_face != "regular") {
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

    return FontSourceHandle(std::string(font.fontName.UTF8String));
}

bool iOSPlatform::startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) {
    __strong TGMapView* mapView = m_mapView;

    UrlResponse errorResponse;
    if (!mapView) {
        errorResponse.error = "MapView not initialized.";
        onUrlResponse(_request, std::move(errorResponse));
        return false;
    }

    id<TGURLHandler> urlHandler = mapView.urlHandler;

    if (!urlHandler) {
        errorResponse.error = "urlHandler not set in MapView";
        onUrlResponse(_request, std::move(errorResponse));
        return false;
    }

    __weak TGMapView* weakMapView = m_mapView;
    TGDownloadCompletionHandler handler = ^void (NSData* data, NSURLResponse* response, NSError* error) {

        __strong TGMapView* mapView = weakMapView;
        if (!mapView) {
            // Map was disposed before the request completed, so abort the completion handler.
            return;
        }

        // Create our response object. The '__block' specifier is to allow mutation in the data-copy block below.
        __block UrlResponse urlResponse;

        // Check for errors from NSURLSession, then check for HTTP errors.
        if (error != nil) {

            urlResponse.error = [error.localizedDescription UTF8String];

        } else if ([response isKindOfClass:[NSHTTPURLResponse class]]) {

            NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
            long statusCode = [httpResponse statusCode];
            if (statusCode < 200 || statusCode >= 300) {
                urlResponse.error = [[NSHTTPURLResponse localizedStringForStatusCode: statusCode] UTF8String];
            }
        }

        // Copy the data from the NSURLResponse into our URLResponse.
        // First we allocate the total data size.
        urlResponse.content.resize([data length]);
        // NSData may be stored in several ranges, so the 'bytes' method may incur extra copy operations.
        // To avoid that we copy the data in ranges provided by the NSData.
        [data enumerateByteRangesUsingBlock:^(const void * _Nonnull bytes, NSRange byteRange, BOOL * _Nonnull stop) {
            memcpy(urlResponse.content.data() + byteRange.location, bytes, byteRange.length);
        }];

        // Run the callback from the requester.
        onUrlResponse(_request, std::move(urlResponse));
    };

    NSString* urlAsString = [NSString stringWithUTF8String:_url.string().c_str()];
    NSURL* url = [NSURL URLWithString:urlAsString];

    _id = [urlHandler downloadRequestAsync:url completionHandler:handler];

    return true;
}

void iOSPlatform::cancelUrlRequestImpl(const UrlRequestId _id) {
    __strong TGMapView* mapView = m_mapView;

    if (!mapView) {
        return;
    }

    id<TGURLHandler> urlHandler = mapView.urlHandler;

    if (!urlHandler) {
        return;
    }

    [urlHandler cancelDownloadRequestAsync:_id];
}

} // namespace Tangram
