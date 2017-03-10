#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Tangram {

// Function type for a mapReady callback
using MapReady = std::function<void(void*)>;

// Function type for receiving data from a successful network request
using UrlCallback = std::function<void(std::vector<char>&&)>;

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

    virtual std::string resolveAssetPath(const std::string& path) const;

    // Read a file as a string
    // Opens the file at the _path and returns a string with its contents.
    // If the file cannot be found or read, the returned string is empty.
    virtual std::string stringFromFile(const char* _path) const;

    // Read a file into memory
    // Opens the file at _path then allocates and returns a pointer to memory
    // containing the contents of the file. The size of the memory in bytes is written to _size.
    // If the file cannot be read, nothing is allocated and nullptr is returned.
    virtual std::vector<char> bytesFromFile(const char* _path) const;

    // Start retrieving data from a URL asynchronously
    // When the request is finished, the callback _callback will be
    // run with the data that was retrieved from the URL _url
    virtual bool startUrlRequest(const std::string& _url, UrlCallback _callback) = 0;

    // Stop retrieving data from a URL that was previously requested
    virtual void cancelUrlRequest(const std::string& _url) = 0;

    virtual std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const;

    virtual std::vector<FontSourceHandle> systemFontFallbacksHandle() const;

protected:

    bool bytesFromFileSystem(const char* _path, std::function<char*(size_t)> _allocator) const;

private:

    bool m_continuousRendering;

};

} // namespace Tangram
