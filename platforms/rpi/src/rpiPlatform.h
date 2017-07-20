#pragma once

#include "platform.h"
#include "urlClient.h"

namespace Tangram {

class RpiPlatform : public Platform {

public:

    RpiPlatform();
    RpiPlatform(UrlClient::Options urlClientOptions);
    ~RpiPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;

protected:

    UrlClient m_urlClient;

};

} // namespace Tangram
