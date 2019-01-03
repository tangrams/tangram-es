#include "platform.h"
#include "log.h"

#include <fstream>
#include <string>
#include <cassert>

namespace Tangram {

Platform::Platform() : m_continuousRendering(false) {}

Platform::~Platform() {}

void Platform::setContinuousRendering(bool _isContinuous) {
    m_continuousRendering = _isContinuous;
}

bool Platform::isContinuousRendering() const {
    return m_continuousRendering;
}

bool Platform::bytesFromFileSystem(const char* _path, std::function<char*(size_t)> _allocator) {
    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        LOGW("Failed to read file at path: %s", _path);
        return false;
    }

    size_t size = resource.tellg();
    char* cdata = _allocator(size);

    resource.seekg(std::ifstream::beg);
    resource.read(cdata, size);
    resource.close();

    return true;
}

FontSourceHandle Platform::systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const {
    // No-op by default
    return FontSourceHandle();
}

std::vector<FontSourceHandle> Platform::systemFontFallbacksHandle() const {
    // No-op by default
    return {};
}

void Platform::shutdown() {
    if (m_shutdown.exchange(true)) { return; }

    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);

        UrlResponse response;
        response.error = "Shutting down";

        for (auto& entry : m_urlCallbacks) {
            auto& request = entry.second;
            if (request.callback) {
                request.callback(std::move(response));
            }
            if (request.id) {
                urlRequestCanceled(request.id);
            }
        }
        m_urlCallbacks.clear();
    }
}

UrlRequestHandle Platform::startUrlRequest(Url _url, UrlCallback&& _callback) {
    assert(_callback);

    if (m_shutdown) {
        UrlResponse response;
        response.error = "Shutting down";
        if (_callback) {
            _callback(std::move(response));
        }
        return 0;
    }

    UrlRequestHandle handle= ++m_urlRequestCount;

    // Need to do this in advance in case startUrlRequest calls back synchronously..
    UrlRequestEntry* entry = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        auto it = m_urlCallbacks.emplace(handle, UrlRequestEntry{std::move(_callback), 0});
        entry = &it.first->second;
    }

    // Start Platform specific url request
    entry->id = startUrlRequest(_url, handle);

    if (entry->id == UrlRequestNotCancelable) {
        LOGW("Cannot cancel request for handle %llu! %s", handle, _url.path().c_str());
    }

    return handle;
}

void Platform::cancelUrlRequest(UrlRequestHandle _request) {
    UrlRequestId id = 0;

    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        auto it = m_urlCallbacks.find(_request);
        if (it != m_urlCallbacks.end()) {
            id = it->second.id;
        }
    }

    if (id != UrlRequestNotCancelable) {
         urlRequestCanceled(id);
    }
}

void Platform::onUrlResponse(UrlRequestHandle _request, UrlResponse&& _response) {
    if (m_shutdown) {
        LOGW("onUrlResponse after shutdown");
        return;
    }
    // Find the callback associated with the request.
    UrlCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        auto it = m_urlCallbacks.find(_request);
        if (it != m_urlCallbacks.end()) {
            callback = std::move(it->second.callback);
            m_urlCallbacks.erase(it);
        }
    }
    if (callback) { callback(std::move(_response)); }
}

} // namespace Tangram
