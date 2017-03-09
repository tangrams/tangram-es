#pragma once

#include "platform.h"
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#include <mutex>

#ifdef __OBJC__
#import <Foundation/Foundation.h>

namespace Tangram {

class OSXPlatform : public Platform {

public:

    OSXPlatform();
    ~OSXPlatform() override;
    void requestRender() const override;
    std::string stringFromFile(const char* _path) const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;

private:

    std::mutex m_urlRequestMutex;
    bool m_stopUrlRequests;
    NSURLSession* m_defaultSession;

};

} // namespace Tangram

#endif
