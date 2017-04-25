#pragma once

#include "util/url.h"

#include <functional>
#include <string>
#include <vector>

namespace Tangram {


// Identifier type for URL requests. This handle is interpreted differently for
// each platform type, so do not perform any application logic with its value.
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
using UrlCallback = std::function<void(UrlResponse)>;

using FontSourceLoader = std::function<std::vector<char>()>;

struct FontSourceHandle {
    FontSourceHandle(std::string _path) : path(_path) {}
    FontSourceHandle(FontSourceLoader _loader) : load(_loader) {}

    std::string path;
    FontSourceLoader load;
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
    virtual UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) = 0;

    // Stop retrieving data from a URL that was previously requested. When a
    // request is canceled its callback will still be run, but the response
    // will have an error string and the data may not be complete.
    virtual void cancelUrlRequest(UrlRequestHandle _request) = 0;

    virtual std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const;

    virtual std::vector<FontSourceHandle> systemFontFallbacksHandle() const;

protected:

    static bool bytesFromFileSystem(const char* _path, std::function<char*(size_t)> _allocator);

private:

    bool m_continuousRendering;

};

} // namespace Tangram
