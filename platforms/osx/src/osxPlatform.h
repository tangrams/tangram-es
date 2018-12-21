#pragma once

#include "platform.h"

#ifdef __OBJC__
#import <Foundation/Foundation.h>

namespace Tangram {

class OSXPlatform : public Platform {

public:

    OSXPlatform();
    ~OSXPlatform() override;
    void shutdown() override {}
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    UrlRequestId startUrlRequest(Url _url, UrlRequestHandle _request) override;
    void urlRequestCanceled(UrlRequestId _id) override;

    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;

private:

    NSURLSession* m_urlSession;

};

} // namespace Tangram

#endif
