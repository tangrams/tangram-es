#pragma once

#include "platform.h"
#include "urlClient.h"

namespace Tangram {

class WindowsPlatform : public Platform {

public:

    WindowsPlatform();
    WindowsPlatform(UrlClient::Options urlClientOptions);
    ~WindowsPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _request) override;

protected:

    UrlClient m_urlClient;

};

} // namespace Tangram
