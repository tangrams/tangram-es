#pragma once

#include "platform.h"
#include "urlClient.h"

namespace Tangram {

class LinuxPlatform : public Platform {

public:

    LinuxPlatform();
    LinuxPlatform(UrlClient::Options urlClientOptions);
    ~LinuxPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _request) override;

protected:

    UrlClient m_urlClient;

};

} // namespace Tangram
