#pragma once

#include "util/url.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Tangram {

// Handle for URL requests.
// This is the handle which Platform uses to identify an UrlRequest.
using UrlRequestHandle = uint64_t;

// Result of a URL request. If the request could not be completed or if the
// host returned an HTTP status code >= 400, a non-null error string will be
// present. This error string is only valid in the scope of the UrlCallback
// that provides the response. The content of the error string may be different
// for each platform type.
struct UrlResponse {
    std::vector<char> content;
    const char* error = nullptr;
};

// Function type for receiving data from a URL request.
using UrlCallback = std::function<void(UrlResponse&&)>;

using FontSourceLoader = std::function<std::vector<char>()>;

struct FontSourceHandle {

    FontSourceHandle() {}
    ~FontSourceHandle() {}

    explicit FontSourceHandle(Url path) : fontPath(path) { tag = FontPath; }
    explicit FontSourceHandle(std::string name) : fontName(name) { tag = FontName; }
    explicit FontSourceHandle(FontSourceLoader loader) : fontLoader(loader) { tag = FontLoader; }

    enum { FontPath, FontName, FontLoader, None } tag = None;
    Url fontPath;
    std::string fontName;
    FontSourceLoader fontLoader;

    bool isValid() const { return tag != None; }
};

// Print a formatted message to the console
// Uses printf syntax to write a string to stderr (or logcat, on Android)
void logMsg(const char* fmt, ...);

void initGLExtensions();

// Set the priority of the current thread. Priority is equivalent to pthread niceness
void setCurrentThreadPriority(int priority);

class Platform {

public:

    Platform();
    virtual ~Platform();

    // Subclasses must call Platform::shutdown() when overriding shutdown
    virtual void shutdown();

    // Request that a new frame be rendered by the windowing system
    virtual void requestRender() const = 0;

    // If called with 'true', the windowing system will re-draw frames continuously;
    // otherwise new frames will only be drawn when 'requestRender' is called.
    virtual void setContinuousRendering(bool _isContinuous);

    virtual bool isContinuousRendering() const;

    // Start retrieving data from a URL asynchronously. When the request is
    // finished, the callback _callback will be run with the data or error that
    // was retrieved from the URL _url. The callback may run on a different
    // thread than the original call to startUrlRequest.
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback&& _callback);

    // Stop retrieving data from a URL that was previously requested. When a
    // request is canceled its callback will still be run, but the response
    // will have an error string and the data may not be complete.
    void cancelUrlRequest(UrlRequestHandle _request);

    virtual FontSourceHandle systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const;

    virtual std::vector<FontSourceHandle> systemFontFallbacksHandle() const;

protected:
    // Platform implementation specific id for URL requests. This id is
    // interpreted differently for each platform type, so do not perform any
    // application logic with its value.
    // It's purpose is to be able to cancel UrlRequests
    using UrlRequestId = uint64_t;

    // To be called by implementations to pass UrlResponse
    void onUrlResponse(UrlRequestHandle _request, UrlResponse&& _response);

    virtual void cancelUrlRequestImpl(UrlRequestId _id) = 0;

    // Return true when UrlRequestId has been set (i.e. when request is async and can be canceled)
    virtual bool startUrlRequestImpl(const Url& _url, UrlRequestHandle _request, UrlRequestId& _id) = 0;

    static bool bytesFromFileSystem(const char* _path, std::function<char*(size_t)> _allocator);

    std::atomic<bool> m_shutdown{false};

    bool m_continuousRendering;

    std::mutex m_callbackMutex;
    struct UrlRequestEntry {
        UrlCallback callback;
        UrlRequestId id;
        bool cancelable;
    };
    std::unordered_map<UrlRequestHandle, UrlRequestEntry> m_urlCallbacks;
    std::atomic_uint_fast64_t m_urlRequestCount = {0};
};

} // namespace Tangram
